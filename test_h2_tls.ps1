$ErrorActionPreference = "Stop"
try {
    Write-Host "Testing HTTP/2 over TLS..."
    # Try to find curl.exe
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        & $curl -k -v --http2 https://localhost:8080/api/hello
    } else {
        Write-Host "curl.exe not found. Cannot test HTTP/2."
    }
} catch {
    Write-Host "Error: $_"
}
