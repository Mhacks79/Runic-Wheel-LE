# Security policy

Runic Wheel LE is a native DLL plugin that interacts with a game's process. Input hooks and memory access deserve careful testing.

- Report a crash or possible security issue to the maintainer privately before publication when it may involve memory corruption, arbitrary code execution or unsafe native hooks.
- Do not attach save files containing sensitive information or proprietary Bethesda game executables to public issues.
- Reproduce problems with a clean game profile and known-compatible SKSE version.
- No support or security guarantees are offered for modified DLLs, unofficial builds or SE/AE binaries.
- Build verification and static checks do not prove runtime safety.

Contact: [Mhacks79 on GitHub](https://github.com/Mhacks79). You may open an issue for non-sensitive crashes.
