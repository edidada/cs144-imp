"""Compile/run only on Actions; retain original Checkpoint 0 tests and logs."""
import hashlib
import json
import os
from pathlib import Path
import socket
import subprocess
import threading
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'reports/check0-results'


def run(name, args, timeout=600):
    result = subprocess.run(args, cwd=ROOT, capture_output=True, timeout=timeout)
    (OUT / (name + '.log')).write_bytes(result.stdout + b'\nSTDERR\n' + result.stderr)
    if result.returncode:
        raise RuntimeError(f'{name} failed ({result.returncode}); inspect log')
    return result


def loopback():
    # The required webget interface uses port 80; Actions container runs as root.
    payloads = [b'HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nfragmented\x00body',
                b'HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n' + bytes(range(256)) * 8192]
    results = []
    for payload in payloads:
        errors = []
        with socket.socket() as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(('127.0.0.1', 80))
            server.listen(1)
            server.settimeout(15)

            def respond():
                try:
                    connection, _ = server.accept()
                    with connection:
                        connection.settimeout(15)
                        request = b''
                        while b'\r\n\r\n' not in request:
                            part = connection.recv(4096)
                            if not part:
                                raise RuntimeError('EOF before complete HTTP request')
                            request += part
                            if len(request) > 8192:
                                raise RuntimeError('oversized request')
                        assert request == b'GET /fixture HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n'
                        for offset in range(0, len(payload), 317):
                            connection.sendall(payload[offset:offset + 317])
                except Exception as error:
                    errors.append(repr(error))

            thread = threading.Thread(target=respond, daemon=True)
            thread.start()
            try:
                result = subprocess.run([str(ROOT / 'build/apps/webget'), '127.0.0.1', '/fixture'],
                                        capture_output=True, timeout=20)
            finally:
                thread.join(timeout=16)
            assert not thread.is_alive() and not errors, errors
            assert result.returncode == 0 and result.stdout == payload, 'HTTP response bytes differ'
            results.append({'bytes': len(payload), 'sha256': hashlib.sha256(payload).hexdigest(), 'passed': True})
    return results


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    summary = {'commit': os.environ.get('GITHUB_SHA'), 'passed': False,
               'scope': 'Fall 2026 public Checkpoint 0; not private grader, SMTP/email or later checkpoints'}
    try:
        run('toolchain', ['g++', '--version'])
        run('configure', ['cmake', '-S', '.', '-B', 'build', '-G', 'Ninja'])
        run('build', ['cmake', '--build', 'build', '--parallel', '2', '--target',
                      'functionality_testing', 'webget', 'speed_testing', 'check0_extra'])
        # Keep upstream check0 invocation intact, including live Stanford HTTP hash test.
        run('official-check0', ['cmake', '--build', 'build', '--target', 'check0'])
        run('junit', ['ctest', '--test-dir', 'build', '-R',
                      '^byte_stream_|^no_skip$|^t_webget$|^check0_extra$',
                      '--output-on-failure', '--timeout', '30', '--output-junit', str(OUT / 'junit.xml')])
        tests = ET.parse(OUT / 'junit.xml').getroot().findall('.//testcase')
        assert len(tests) == 12, f'Expected 12 tests including compile fixtures, got {len(tests)}'
        assert all(t.get('status') == 'run' and t.find('failure') is None and t.find('skipped') is None for t in tests)
        summary['ctest'] = [{'name': t.get('name'), 'time': t.get('time')} for t in tests]
        summary['http_loopback'] = loopback()
        summary['passed'] = True
    finally:
        (OUT / 'results.json').write_text(json.dumps(summary, indent=2) + '\n')
    print('Checkpoint 0: upstream check0, CTest/ASan/UBSan, speed and binary HTTP checks passed')


if __name__ == '__main__':
    main()
