# DSHBA

[![CodeFactor](https://www.codefactor.io/repository/github/densinh/dshba/badge)](https://www.codefactor.io/repository/github/densinh/dshba)

After writing [my GBA emulator, GBAC-, found here](https://github.com/DenSinH/GBAC-), I wanted to write a new one, but faster.
One extra challenge I wanted to add was writing a hardware renderer.

<img src="https://github.com/DenSinH/DSHBA/blob/master/files/DSHBA_Ruby.png" alt="Pokemon Ruby" width=640>

### Requirements

To build, you need `SDL`, and optionally `capstone[arm]`. `ImGui` and `glad` are included in the project. You need a GPU that supports at least OpenGL 3.3 to run this project.

### Description

So I did exactly that. I rewrote it in C++, with a lot of optimizations. Some examples:
  - Templated instruction handlers (this is a big time save)
  - Faking the pipeline (so not emulating the pipeline, unless its effects are actually needed)
  - Fast DMAs (straight memcpy where possible)
  - Faster rendering (see below)
  - Better scheduler, with a circular timer. Basically, instead of using a 64 bit system clock, I used an i32, so that at a set time (0x7000'0000), I can reset the time
    First, I was using a u64, but since I compile for 32 bits, this took 2 instructions to tick to, now just 1. This might not sound like much, but it doubled the framerate in a lot of games.
  - Batching up APU ticking
  - Fast (decent, but not nearly perfect) open bus emulation. Basically, when open bus reads happen, I calculate back what the bus value should have been.
  - Memory read page tables. For a lot of memory regions, reading is very straightforward, so I added a pagetable for them (iWRAM/eWRAM/PAL/VRAM/OAM/ROM)
  - Compiler intrinsics. I wrote a bunch of wrappers for some common stuff, and tried to mark some edge case branches as likely or unlikely with it.
  
I spent quite a bit of time using the Intel VTune profiler to see what parts actually took a lot of time, and optimized this stuff out.
  
### The hardware renderer

I used OpenGL (3.3 core) for this. Writing the hardware renderer was a lot of fun, I could copy a lot of code from my old GBA emulator's PPU code.
An extra special feature is affine background/sprite upscaling. Basically, instead of rendering at the GBA's resolution (240x160), I render at 4 times that resolution (480x320),
and handle affine transforms on a sub-pixel level. 
This allows for much crisper affine sprites.

One of the biggest challenges was adding alpha blending. The way the GBA handles alpha blending (with different top/bottom layers that only blend with each other), does not map well to
modern GPUs. What I did to solve this was render everything twice basically (only when blending is actually enabled though), In one layer, render everything, in the other, render only all non-top layers.
Then in an extra pass, blend those 2 layers together.

There are a bunch of shaders involved:
  - background shaders (vertex, fragment, fragment for each of the modes)
  - object shaders (vertex, fragment), also used for object window
  - window shaders (vertex, fragment), for the basic windows (WIN0, WIN1, WINOUT)
  - blitting shader (vertex, fragment), for the final rendering pass.

This took a bit more GPU power, but I think the most GPU power consumption comes from buffering the data. 

In the hardware renderer I also added a lot of optimizations:
  - I batch scanlines. Basically, whenever a write to VRAM happens, I save what range of VRAM was dirtied, and when I render, I batch up scanline with the same VRAM state.
    Usually, games are nice and I can render full screens at once, saving a lot of buffering
  - Object batching. I do basically the same thing for the OAM memory. Most of the very little video parsing I do on the CPU is parsing what objects need to be drawn, and send separate 
    draw calls for those. I batch this process up in the same way I do scanlines, except I dont keep track of a range, but buffer all of OAM every time (after all, OAM is kind of small).
    When writes to the object part of VRAM happen, OAM is also marked as dirty.
    This optimization also allowed me to add a GBA hardware quirk: OAM update delay. Basically, updates to OAM only go into effect the scanline after. Adding this was simply changing the
    OAM dirty marking from a single bool to 2, one for this scanline and one for the last one.
  - Palette RAM batching. Basically, when a write to palette RAM happens, I buffer it anew into the corresponding texture. I also set an index as to what buffer should be looked at for palette RAM in the shader.
    Those are sent to the shader in the form of a uniform array. This allows me to buffer palette RAM only once per frame.
  - Color Correction. Not really an optimization, but it looks really nice. Credits to Byuu/Near for the color correction algorithm (link is in the shader).
  - [OPTIONAL] Frameksipping. Frameskipping didn't affect emulator performance much, it mostly saves a lot of GPU usage.
    
 The hardware renderer save a lot of CPU usage, but these extra optimizations also saved a lot of CPU time, and gained me some extra performance in the process.
 
 ### The Cached Interpreter
 I recently decided that I wanted to try and write a cached interpreter. I spent some time thinking
 and writing the code, and after I got it working, I gained another 10-20% boost in most games. 
 
 The basic idea for the cached interpreter is that you want to skip the expensive memory reads and
 instruction decodes when you run code. Especially in the ROM and BIOS regions this is extremely useful,
 since those regions cannot be written to, and the code will always be the same. 
 
 In iWRAM this is a bit different. It's very common for games to run code in iWRAM. What I did
 was have an "instruction cache page table". Basically, the instruction caches are limited to 256 bytes, aligned
 by 256 byte page boundaries. Whenever a write to an iWRAM location happens, I look in the page table
 to see how which addresses are filled (just a vector with those addresses), and clear those addresses.
 Usually, I would expect there to be at most 1 or 2 blocks in that region (perhaps some more if there are a lot of short branches).
 Since the stack is also in iWRAM, and I don't want to unnecessarily check for blocks on every push/pop, I limit the iWRAM region by a few hunder bytes.
 The number I chose here was pretty arbitrary, and I did not test different values of it. With the cache page tables, clearing the blocks did not take long anyway.
 
 Basically, the run loop is no longer `fetch -> decode -> execute`, but it's now:
 
 1. Check if there is an instruction cache at `pc`
 2. If there is, do the following for every instruction in the cache:
    * check if scheduler events happen that affect the CPU (IRQs)
        * If not, execute the next instruction in the cache
        * If yes, break the block early and go to step 1.
 3. If there is not, check if we can make a cache (in ROM/BIOS/iWRAM regions). Then do the following:
    * check if scheduler events happen that affect the CPU (IRQs)
        * If not, `fetch -> decode` the next instruction. Store it in the current
          cache block (pointer to call and instruction).
            * If the instruction was a branch, or affects pc, then end the block and return to step 1.
            * If the instruction was not a branch, keep doing this. 
        * If the scheduler did interrupt the CPU, then throw away the block (we want them to be as long as possible)
          and return to step 1.
 4. If we cannot make a cache in this region (not ROM/BIOS/iWRAM), keep running "the old fashioned way", until we can, or
    a cache block is available.   
    
 
 ### Compatibility
 
 I have tried a bunch of games, most worked fine, some with a few graphical glitches. As for accuracy: I pass all AGS tests, except the ones requiring very accurate timings
 (those are: the last 3 memory tests, testing waitstates and the prefetch buffer, and the timer tests, requiring cycle accuracy). 
 
 ### Controls
 
 Default controls are:
 
 GBA    | Keyboard | Gamepad
 -------|----------|---------
 A      |   Z/C    | A / X
 B      |   X/V    | B / Y
 Up     |    Up    | Dpad Up
 Down   |   Down   | Dpad Down
 Left   |   Left   | Dpad Left
 Right  | Right    | Dpad Right
 Start  | A        | Start
 Select | S        | Select
 L      | Q        | L
 R      | R        | R
 
 If you don't like the controls/want to change them, you can edit the `input.map` file.
 
 ### The performance
 
 I am really proud of the performance of my newly rewritten emulator. On my fairly old system (intel i7 2600 and a GTX670). On Pokemon Emerald (notoriously slow for using waitloops),
 I got framerates of about 1000fps on the intro sequence and about 900 in game (as a reference: this is more than mGBA!). Some games gave me insane performance though:
 the GTA menu screen spiked over 10k fps and the Zelda menu gave me framerates of about 5kfps, the highest framerate I've seen is in the Doom menu screen, at 17k fps.
 
 ![Zelda menu screen](https://github.com/DenSinH/DSHBA/blob/master/files/DSHBA_unlocked.png)
 
 In the above screenshot, you can see the emulator in its full potential, with alphablending and everything.
 
 ### Debugger
 
The UI and the debugger are written in ImGui. I tried to keep them as generic as possible, that way I could re-use them for other emulator projects I might do. On release builds, not all the console commands work, the memory viewer still should, and so should the overlay and the register viewer. The decompiler needs `capstone.dll`, If you build without capstone installed in your package manager, it won't try to link it, and should just say `"Decompiling unavailable"` in the window.

### BIOS

I have included the replacement BIOS that Fleroviux and I made for the GBA (repo is [here](https://github.com/Cult-of-GBA/BIOS)). If you want to use a different file, you will have to build yourself, and uncomment the `gba->LoadBIOS("path");` call and add the right path to your BIOS file, or change the `BIOS_FILE` macro and uncomment that call. The replacement BIOS should have all functionality the official one has that is used in games.
