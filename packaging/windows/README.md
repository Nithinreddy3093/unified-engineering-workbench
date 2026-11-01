# Windows packaging plan

The current Windows executable is not packaged. This folder records the intended release workflow so it is not confused with a verified portable package.

1. Build x64 Release on Windows 11 using the documented C++17 toolchain.
2. Run CTest and the UI acceptance checks on that build.
3. Audit all runtime dependencies and licenses. Current intended dependencies are Windows system libraries only.
4. Stage the executable, `data/`, and relative `results/` directory in a clean staging folder. Confirm the process working directory does not depend on the source tree.
5. Provide a small native `Launch.exe` that sets the application working directory to its own folder and starts the app. Do not require PowerShell or first-run compilation.
6. Zip the staging folder and test it on a clean Windows 11 x64 account/VM without developer tools, elevation, or network access.
7. Record the archive hash, test machine OS/build, launch result, and screenshots in `docs/evidence/`.

No `Launch.exe`, release archive, dependency bundle, or clean-machine test is included yet. Packaging must remain marked incomplete until those steps are performed.
