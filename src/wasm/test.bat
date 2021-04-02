emcc.bat -s USE_SDL=2 .\src\wasm\main.c .\src\core\bus.c .\src\core\apu\apu.c .\src\core\apu\apu_io.c .\src\core\apu\noise.c .\src\core\apu\wave.c .\src\core\apu\square1.c .\src\core\apu\square2.c .\src\core\cpu.c .\src\core\gb.c .\src\core\joypad.c .\src\core\serial.c .\src\core\sgb.c .\src\core\timers.c .\src\core\ppu\dmg_renderer.c .\src\core\ppu\gbc_renderer.c .\src\core\ppu\ppu.c .\src\core\ppu\sgb_renderer.c .\src\core\mbc\mbc.c .\src\core\mbc\mbc_0.c .\src\core\mbc\mbc_1.c .\src\core\mbc\mbc_2.c .\src\core\mbc\mbc_3.c .\src\core\mbc\mbc_5.c .\src\core\tables\palette_table.c -I .\src\ -o index.html -DGB_SDL_AUDIO_CALLBACK_STREAM --shell-file src/wasm/shell_minimal.html -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" -s WASM=1 -O3 -DNDEBUG