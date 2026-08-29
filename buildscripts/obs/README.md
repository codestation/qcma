# OBS project configuration

`project-config` resolves ambiguous FFmpeg providers by selecting the regular
libraries. In particular, the `62` entries fix Ubuntu 26.04 choosing between
`libavcodec62`/`libavcodec-extra62` and
`libavformat62`/`libavformat-extra62`.

Review and apply it with:

```sh
osc meta prjconf home:codestation -e
```

Do not replace unrelated project configuration added after this file was
created. `appimage-repositories.xml` contains the x86_64 repositories to add
to the project metadata when enabling AppImage. Leap 16 is deliberately used
as the RPM base instead of Tumbleweed to avoid bundling binaries that require
a newer glibc than Ubuntu 24.04.
