import os
import sys
import struct

def make_initrd(files, output_filename):
    with open(output_filename, 'wb') as f:
        # Number of files
        f.write(struct.pack('<I', len(files)))
        
        offset = 4 + len(files) * (32 + 4) # Start of data segment
        
        headers = []
        data_blobs = []
        
        for filepath in files:
            name = os.path.basename(filepath)
            name_bytes = name.encode('ascii')[:31]
            name_padded = name_bytes + b'\x00' * (32 - len(name_bytes))
            
            with open(filepath, 'rb') as f_in:
                data = f_in.read().replace(b'\r\n', b'\n')
                if not data.endswith(b'\n'):
                    data += b'\n'
                size = len(data)

                
            headers.append(name_padded + struct.pack('<I', size))
            data_blobs.append(data)
            
        for h in headers:
            f.write(h)
        for d in data_blobs:
            f.write(d)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: mkinitrd.py output.img file1 file2...")
        sys.exit(1)
    make_initrd(sys.argv[2:], sys.argv[1])
    print(f"Created initrd {sys.argv[1]} with {len(sys.argv)-2} files.")
