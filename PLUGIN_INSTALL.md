# Plugin install + rescan field manual

This is the quick-and-dirty field sheet for dropping the Horizon plugins where your DAW can see them, force-rescanning when the host gets grumpy, and debugging the usual “why is nothing coming out?” questions. Keep it handy on sessions.

## Where to drop the binaries

- **macOS**
  - VST3: `/Library/Audio/Plug-Ins/VST3`
  - Audio Units (AU): `/Library/Audio/Plug-Ins/Components`
  - AAX (Pro Tools): `/Library/Application Support/Avid/Audio/Plug-Ins`
- **Windows**
  - VST3: `C:\Program Files\Common Files\VST3`
  - AAX: `C:\Program Files\Common Files\Avid\Audio\Plug-Ins`
- **Linux**
  - VST3: `/usr/lib/vst3` (system) or `~/.vst3` (per-user)

> Pro tip: if you keep portable builds on a USB stick, mirror this layout so you can drag-drop without thinking.

## How to force a rescan (host-by-host)

- **Ableton Live**: Preferences → Plug-Ins → toggle “Use VST3 Plug-In System Folders” off/on, then hit “Rescan” or hold *Cmd/Ctrl* when launching to trigger a full scan.
- **Reaper**: Options → Preferences → Plug-ins → VST → “Clear cache/re-scan” (or “Re-scan” if you just moved files around). Make sure the VST paths include the folders above.
- **Logic Pro**: AU only. Move the component into `/Library/Audio/Plug-Ins/Components`, then run `auval -a` in Terminal or just restart Logic; it will validate on launch.
- **Bitwig**: Settings → Locations → Plug-ins → hit “Rescan” under VST3. If you run sandboxed plug-ins, watch the bottom status bar for validation errors.
- **Pro Tools**: Put the AAX in `Avid/Audio/Plug-Ins` and trash `InstalledAAXPlugins` from `~/Library/Preferences/Avid/Pro Tools/` (macOS) or `%AppData%\Avid\Pro Tools\` (Windows) to force a rebuild.

If the plugin still hides, confirm you’re on a build that matches the host architecture (ARM vs x86) and that the host is scanning VST3/AU/AAX, not just VST2.

## Troubleshooting the usual suspects

### “Why is there no sound?”
- Verify the plugin is inserted on an *audio* track (or instrument return) and that the track is not muted or routed to nowhere.
- Check the plugin input meters. If they’re flat, the host isn’t feeding us; check the track’s input selector and monitoring mode.
- Bypass the plugin. If audio returns, check Mix/Output in the plugin: set Mix to 100%, Output to unity, and disable any safety mute/limiter you might have toggled.
- On macOS, confirm the plugin passed AU validation (`auval -t aufx H0RZ Manu` style). On Windows, make sure the VC runtime that shipped with the build is installed.

### “Why did my stereo go mono?”
- Insert on a stereo track and confirm the host actually delivers two channels (some DAWs default to mono unless you pick a stereo input).
- If you’re mid/side encoding before Horizon, make sure you decode after; otherwise the mix will sound like a collapsed mono sum.
- Disable any “Mono” or “Dual Mono” mode the host might have forced (Logic’s channel strip and Pro Tools’ multi-mono inserts can do this).
- Check the plugin preset: if you cranked Width/Dyn Width to zero, you intentionally collapsed the stage—dial them back up.

### “Why is there latency?”
- The limiter and safety lookahead add a few milliseconds. Hosts that report plugin delay compensation (PDC) should auto-align; if they don’t, bump the track by the reported number of samples.
- Large host buffers (1024+ samples) will make the plugin feel laggy on live inputs. Drop the buffer to 64–256 for tracking, then raise it for mixdown.
- If you hear phasing, make sure you’re not monitoring both the dry input and the plugin return simultaneously (disable direct monitoring or mute the input channel).

### “It still feels off”
- Swap USB cables/audio interfaces before you burn time—clocking and driver issues masquerade as plugin problems.
- Make a quick before/after bounce to confirm the plugin is actually in the path and doing what you think. If the files null, you might be hearing placebo.

Keep this page rough and actionable: the goal is to get sound out of the monitors, fast.
