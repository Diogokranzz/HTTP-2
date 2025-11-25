$ErrorActionPreference = "Stop"
try {
    Write-Host "Testing HTTPS..."
    # Try to find curl.exe
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        & $curl -k -v https://localhost:8080/api/hello
    } else {
        # Fallback to .NET
        [System.Net.ServicePointManager]::ServerCertificateValidationCallback = {$true}
        $wc = New-Object System.Net.WebClient
        $wc.DownloadString("https://localhost:8080/api/hello")
    }
} catch {
    Write-Host "Error: $_"
}
