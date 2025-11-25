$client = New-Object System.Net.Sockets.TcpClient("localhost", 8080)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)

$req = "GET /api/hello HTTP/1.1`r`nHost: localhost`r`nConnection: keep-alive`r`n`r`n"
$buffer = [System.Text.Encoding]::ASCII.GetBytes($req)

$start = Get-Date

for ($i = 0; $i -lt 1000; $i++) {
    $stream.Write($buffer, 0, $buffer.Length)
    # Read headers
    while ($line = $reader.ReadLine()) {
        if ($line -eq "") { break }
    }
    # Read body (assuming 50 bytes for now, simplified)
    $chars = New-Object char[] 50
    $reader.Read($chars, 0, 50) | Out-Null
}

$end = Get-Date
$duration = ($end - $start).TotalSeconds
$rps = 1000 / $duration

Write-Output "RPS: $rps"
$client.Close()
