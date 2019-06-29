# hexext-releases-IDA7.0

https://forum.reverse4you.org/t/hexext-a-plugin-for-extending-hexrays-7-0-via-microcode/10631

Binary releases for hexext, a plugin to improve the output of the hexrays decompiler through microcode manipulation.

Place it in your plugins folder. Ctrl-2 toggles the improved codegen on and off.

32 bit ARM and x86 support

Still doesn't support 7.2 x64 decompiler. 

32-bit ARM and x86 are now supported. The non-single instruction inliner is disabled in x86 currently. Thunk resolution for ARM/ARM64 will be added later on. 

Combination rules are now dumped to the console. In the next release individual combination rules will be toggleable via a gui and various things will be more tweakable.

There may be interrs in this release, but it's pretty hard for me to test for all possible issues that could arise across four different architectures, so if you want this shit to work and have free time please contact me.
