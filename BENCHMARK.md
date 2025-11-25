# Benchmarking & Tuning Guide (Windows 11)

## 1. Stress Test with `wrk` (via WSL2)

Since `wrk` is a Linux tool, the best way to run it on Windows 11 is inside **WSL2** (Windows Subsystem for Linux).

1.  Open your WSL2 terminal (Ubuntu/Debian).
2.  Install wrk: `sudo apt install wrk`
3.  Run the test against your Windows server (use your local IP or `host.docker.internal` if mapped, but usually `localhost` works if standard networking is set, otherwise check `ipconfig` on Windows):

```bash
# Replace <WINDOWS_IP> with your machine's LAN IP (e.g., 192.168.1.x) to avoid localhost loopback optimizations/issues.
wrk -t12 -c400 -d30s http://<WINDOWS_IP>:8080/index.html
```

### Interpreting Results

*   **Latency**:
    *   **Avg**: Average response time. Lower is better. < 1ms is excellent.
    *   **Stdev**: Jitter. High values mean unstable performance.
*   **Req/Sec**: Throughput.
*   **Socket Errors**:
    *   **Connect**: Server refused connection (backlog full?).
    *   **Read/Write**: Connection dropped.
    *   **Timeout**: Server too slow.

## 2. Windows 11 Tuning for C10k

Windows 11 has default limits on ephemeral ports and TCP connections that can block high-concurrency tests. You need to tweak the Registry.

> **⚠️ WARNING**: Back up your Registry before making changes. A reboot is required for these to take effect.

### PowerShell Script (Run as Administrator)

Run this script to apply the necessary registry keys for high performance:

```powershell
# 1. Increase Max Ephemeral Ports (MaxUserPort)
# Default is usually 5000. We set it to 65534 to allow more outgoing connections (for clients) and handle rapid reuse.
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -Name "MaxUserPort" -Value 65534 -PropertyType DWORD -Force

# 2. Reduce TIME_WAIT Delay (TcpTimedWaitDelay)
# Default is 240s (4 mins). We reduce to 30s to free up ports faster.
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -Name "TcpTimedWaitDelay" -Value 30 -PropertyType DWORD -Force

# 3. Increase TCB (Transmission Control Block) Table Size
# Helps manage state of many concurrent connections.
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -Name "MaxFreeTcbs" -Value 65535 -PropertyType DWORD -Force
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" -Name "MaxHashTableSize" -Value 65535 -PropertyType DWORD -Force

Write-Host "Tuning applied. Please REBOOT your machine."
```

### Manual Registry Tuning (RegEdit)

Navigate to: `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters`

| Value Name | Type | Value (Decimal) | Description |
| :--- | :--- | :--- | :--- |
| `MaxUserPort` | DWORD | `65534` | Increases available ephemeral ports. |
| `TcpTimedWaitDelay` | DWORD | `30` | Reduces time a closed connection stays in TIME_WAIT. |
| `MaxFreeTcbs` | DWORD | `65535` | Increases max active connections hash table. |

## 3. Server-Side Tuning (Code)

Ensure your `listen()` backlog is high enough in `src/sys/WindowsIOCP.cpp`:

```cpp
// In submit_accept or init logic
listen(server_socket, SOMAXCONN); // Windows SOMAXCONN is usually 0x7FFFFFFF (huge)
```

My implementation uses `SOMAXCONN`, so the code is ready.
