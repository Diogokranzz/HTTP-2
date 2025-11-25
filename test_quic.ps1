$udpClient = New-Object System.Net.Sockets.UdpClient
$udpClient.Connect("localhost", 8080)
$bytes = [System.Text.Encoding]::ASCII.GetBytes("QUIC Packet Data")
$udpClient.Send($bytes, $bytes.Length)
$udpClient.Close()
Write-Output "Sent QUIC UDP Packet"
