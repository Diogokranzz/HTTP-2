import socket
import ssl
import time

def test_h2():
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE
    context.set_alpn_protocols(['h2', 'http/1.1'])

    conn = context.wrap_socket(socket.socket(socket.AF_INET), server_hostname='localhost')
    try:
        conn.connect(('localhost', 8080))
        print(f"Connected to {conn.getpeername()}")
        print(f"Selected ALPN Protocol: {conn.selected_alpn_protocol()}")

        if conn.selected_alpn_protocol() == 'h2':
            print("Success! ALPN negotiated h2.")
            
            
            preface = b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n'
            settings_frame = b'\x00\x00\x00\x04\x00\x00\x00\x00\x00'
            headers_frame = bytes.fromhex("00000101050000000182") 
            
            conn.sendall(preface + settings_frame + headers_frame)
            print("Sent Preface + SETTINGS + HEADERS frames")

            
            while True:
                data = conn.recv(4096)
                if not data:
                    break
                print(f"Received {len(data)} bytes")
                print(f"Hex: {data.hex()}")
                
              
                offset = 0
                while offset + 9 <= len(data):
                    length = (data[offset] << 16) | (data[offset+1] << 8) | data[offset+2]
                    ftype = data[offset+3]
                    flags = data[offset+4]
                    stream_id = ((data[offset+5] & 0x7F) << 24) | (data[offset+6] << 16) | (data[offset+7] << 8) | data[offset+8]
                    
                    print(f"Frame Type: {ftype}, Length: {length}, Flags: {flags}, Stream ID: {stream_id}")
                    
                    if ftype == 0x0: # DATA
                        payload = data[offset+9 : offset+9+length]
                        print(f"DATA Payload: {payload.decode(errors='ignore')}")
                    
                    offset += 9 + length
                
                
                if b'Hello from HTTP/2' in data:
                    print("Test Passed: Received expected data!")
                    break
            
        else:
            print("Failed: ALPN did not select h2.")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        conn.close()

if __name__ == "__main__":
    test_h2()
