KwlKit - a small library to uncompress kwl - a simple experimental lossy audio compression format
(royalty and patent free)
(c) Martin Sedlak (mar) 2015

library integration: just add KwlKit.cpp to your project
for additional information see Tutorial/KwlToRaw.cpp

"Compress" folder contains my inflate implementation; this can be used instead of zlib
if desired (inflate can be quite useful for other things like png decompression or VFS implementation)

Comparison to Vorbis:
- since Vorbis is much more complex, it naturally offers better quality/size than kwl
- if we can trust PSNR, default kwl encoder should produce something similar (a tiny bit worse)
than 220 kbps Vorbis (while being approximately 1/3 larger in size), but I wouldn't be surprised
if kwl was actually qualitatively worse
- decoding should be ~6% faster than Vorbis (YMMV)
- in my opinion easier to integrate (no separate container unlike ogg)
- design goals were different for kwl, it doesn't support fast seeking and was primarily designed
for realtime streaming in games (44/48kHz stereo)
