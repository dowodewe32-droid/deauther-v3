#!/usr/bin/env python3
import os
import zlib

def compress_file(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    compressed = zlib.compress(data, 9)
    hex_bytes = ', '.join(f'0x{b:02x}' for b in compressed)
    return hex_bytes, len(compressed)

def get_filename_var(filename):
    base = os.path.splitext(os.path.basename(filename))[0]
    base = base.replace('-', '').replace('.', '').replace(' ', '')
    return base

LANG_MAP = {
    'en': 'enlang',
    'hu': 'hulang',
    'ja': 'jalang',
    'nl': 'nllang',
    'fi': 'filang',
    'cn': 'cnlang',
    'ru': 'rulang',
    'pl': 'pllang',
    'uk': 'uklang',
    'de': 'delang',
    'it': 'itlang',
    'fr': 'frlang',
    'in': 'inlang',
    'ko': 'kolang',
    'ro': 'rolang',
    'da': 'dalang',
    'ptbr': 'ptbrlang',
    'cs': 'cslang',
    'tlh': 'tlhlang',
    'es': 'eslang',
    'th': 'thlang',
}

def main():
    web_dir = 'data/web'
    output_file = 'src/webfiles.h'
    
    files = []
    for root, dirs, filenames in os.walk(web_dir):
        for f in filenames:
            filepath = os.path.join(root, f)
            relpath = os.path.relpath(filepath, web_dir)
            files.append((relpath, filepath))
    
    with open(output_file, 'w') as out:
        out.write('#ifndef webfiles_h\n')
        out.write('#define webfiles_h\n\n')
        out.write('#ifdef ESP32\n')
        out.write('#define PROGMEM\n')
        out.write('#define LittleFS SPIFFS\n')
        out.write('#endif\n\n')
        out.write('#define USE_PROGMEM_WEB_FILES\n\n')
        out.write('#ifdef USE_PROGMEM_WEB_FILES\n')
        
        for relpath, filepath in sorted(files):
            basename = os.path.basename(filepath)
            var_name = get_filename_var(basename)
            
            for code, name in LANG_MAP.items():
                if basename.startswith(code + '.lang'):
                    var_name = name
                    break
            
            hex_data, size = compress_file(filepath)
            
            out.write(f'const char {var_name}[] PROGMEM = {{{hex_data}}};\n')
        
        out.write('#endif\n\n')
        out.write('#endif\n')
    
    print(f'Generated {output_file} with {len(files)} files')

if __name__ == '__main__':
    main()
