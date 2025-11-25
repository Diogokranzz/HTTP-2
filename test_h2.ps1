$client = New-Object System.Net.Sockets.TcpClient('localhost', 8080)
$stream = $client.GetStream()
$preface = [System.Text.Encoding]::ASCII.GetBytes("PRI * HTTP/2.0`r`n`r`nSM`r`n`r`n")
$stream.Write($preface, 0, $preface.Length)
Start-Sleep -Seconds 1
$client.Close()
Write-Output "Sent HTTP/2 Preface"
