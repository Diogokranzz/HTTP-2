$udpClient = New-Object System.Net.Sockets.UdpClient
$udpClient.Connect("localhost", 8080)

# Construct QUIC Short Header Packet
# Flags: 0x40 (Short Header, 1-RTT)
# DCID: 8 bytes (0x01..0x08)
# Payload: HTTP/3 HEADERS Frame
#   Type: 0x01 (HEADERS)
#   Length: 0x01
#   QPACK: 0xD1 (Static Table Index 17 -> :method: GET)

$packet = New-Object System.Collections.Generic.List[Byte]
$packet.Add(0x40) # Flags
$packet.AddRange([byte[]](0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08)) # DCID

# HTTP/3 Frame
$packet.Add(0x01) # Type: HEADERS
$packet.Add(0x01) # Length: 1
$packet.Add(0xD1) # QPACK: Static Index 17 (:method: GET)

$bytes = $packet.ToArray()
$udpClient.Send($bytes, $bytes.Length)
$udpClient.Close()
Write-Output "Sent HTTP/3 Packet"
