# Operating-System (Course Project)

## Build (Windows + MSYS2 UCRT64)
1. Install MSYS2 and open **MSYS2 UCRT64**
2. Install toolchain:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-make
   ln -sf /ucrt64/bin/mingw32-make.exe /ucrt64/bin/make.exe
   ```
3. Build & run:
   ```bash
   make
   ./build/fs.exe
   ```

## Notes
- Use feature branches + PRs. Do not push to `master` directly.