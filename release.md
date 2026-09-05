# WebKeyboard for StarCitizen v0.4
After some delay, a new release has been made.
The main feature is a composite USB device, allowing to refer multiple devices in single command.

> [!NOTE]
> This release document contain [the developer appeal to the community](#appeal-to-the-community)

<details>
 <summary>Subsystems version on release</summary>
 PROTOCOL_VERSION 	 0.2 <br>
 STORAGE_VERSION  	 0.4 <br>
 SERVER_VERSION 	 0.4 <br>
 CLIENT_VERSION 	 0.3 <br>
 INCREMENTAL_VERSION 4   <br>
</details>

## Key features:
- Composite USB device, that may include:
  - keyboard
  - mouse
  - joystick0
  - joystick1
- Ability to use mouse axes and buttons.
- Ability to use joystick axes and buttons
- Ability to combine keys from multiple devices in single command (Alt+Mouse5)
- Command parser/interpreter changes
- Further improvement of the connection flow
- Few new controls
- Bug fixes

## Glossary update:
Direct control - a mode in which a limited set of commands (typically axis transition) is executed directly on the USB backend,
bypassing device command queue and its own(Direct control) command sequence. It is used to minimize widgets input latency.

## Details
#### Composite USB device.
This device allow to combine key pressing and axis translation from multiple devices.
The device is configurable at compile time and may include keyboard, mouse, joystick0, joystick1.
By default, wkb include keyboard mouse and one joystick (joystick0). 
Usb composition controlled by `config.h` define
- `INCLUDE_KEYBOARD` including keyboard to you stack, do not disable it unless you are sure what you're doing, true by default
- `INCLUDE_MOUSE` including mouse to you stack, true by default
- `INCLUDE_JOYSTICK0` including joystick0 to you stack, true by default
- `INCLUDE_JOYSTICK1` including joystick1 to you stack, false by default

The keyboard is standard. Have 6 scancodes + 6 modifiers per report. 3 led indicators.
The mouse is XY absolute positioned contain 5 buttons, 2 scrolls (only vertical scroll supported for now)
Both joysticks are same, contain 8 axis with range [0, 2048] and 32 buttons each. Each joystick require calibration before use.

> [!WARNING]
> Current USB device implementation have limitations<br>
> <br>
> Mouse panning not implemented (on parser/interpreter side)<br>
> Non-Direct control transition command like js-axis3(0) will not sync with axis widget and not replicate across clients. This is not ready. <br>
> If command contain multiple devices references it will-be executed in fix order where keyboard press first and release last (if present)
regardless of the order described in the command<br>
> Other, more technical limitations

#### Ability to use mouse axes and buttons.
From this release it is possible to move mouse, scroll, click with standard control.<br>
New commands:
- `mouse-pos(x,y)` where x,y is absolute position in range [0..32767] <b>NOT PIXELS!</b>, note: formula for converting a value to pixels `32767/resolution*offset`
- `mouse-wheel(clicks)` where clicks is number of encoder clicks in range [-127..127], positive scroll down, negative scroll up
- `mouse-bX` where X is btn index in range [0..4] allow to interact with button with the same rules as for the other buttons
- `mouse-left` same as mouse-b0
- `mouse-right` same as mouse-b1
- `mouse-middle` same as mouse-b2
- `mouse-backward` same as mouse-b3
- `mouse-forward` same as mouse-b4

> [!NOTE]
> Direct control (widgets) for mouse position and scroll not implemented in this release. <br>
> Think twice before using mouse positioning, as you likely won't succeed with it in a 3D game interface and implementation limitation

#### Ability to use joystick axes and buttons.
White direct joystick control presentment in app since initial release. But the buttons and axes transition were inaccessible to the control(trigger) until now.
The joystick command are prefixed with joystick name `js0-` for joystick0, and `js-1` for joystick1, the shortened `js-` command is an alias for `js0-`.
New commands:
- `jsN-axisY(value)` where N joystick index or empty,  and value is joystick absolute position in range [0..2048] or some predefined value
like `low` equal to 0, `middle` equal to 1024 and `high` equal to 2048. For example: `js-axis0(low)` equal to `js0-axis0(0)`, `js-axis5(2000)`
- `jsN-bX` where N joystick index or empty, Y is button index in range [0..31] allow to interact with button with the same rules as for the other buttons

> [!WARNING]
> `js-` token is reference to joystick0 and not first active joystick.
>  It is technically possible to build the application with only the joystick1 active (they are identical). In that case, using the `js-` prefix will trigger an error during interpretation.
>  This is subject to change.

#### Ability to combine keys from multiple devices in single command (Alt+Mouse4).
It is simple in it basic form: `+alt+mouse-b4+` it will press `alt` then after few ms press `mouse4` wait for  interaction time (default press)
and then release combination in reverse order. <br>
You can modify command like this `+alt:double+mouse-b4+` or `+alt:long+mouse-b4+` to change interaction type.
Please note that although the modifier `:double`, `:long` is located in the first token of the combination, it modifies the combination as a whole; 
and as such must be consistent within combination border, please refer to [Parser/interpreter changes](#command-parserinterpreter-changes) for more information. <br>
There are three main ways to execute a command on multiple devices:
- Combination contain only buttons will execute it interaction type as usual, no side effects no additional block. The axial data report for affected devices will be sent unchanged for every press and release.
- Combination contain only axial manipulation will block axial device for time of execution. Execution time is fast, The axial data report for affected devices be sent only once and as is.
- Combination that contain both buttons and axial manipulation will block axial device for time of execution. Execution time long, and depend on interaction type (up to few seconds). The axial data report for affected devices will be sent as is for every press and release.

> [!NOTE]
> Non-Direct axial command on device will block device for time of execution, to prevent command state corruption by direct command. <br>
> Any direct control command in that time frame will-be rejected, with no error on client side due to lack of implementation. <br>
> OS may or may not filter any repeated unchanged report.

#### Command parser/interpreter changes.
Parser before 0.4 unable to execute correctly command that contain more than one not-modifier symbol. <br>
Combination `+ctrl+b+1` before 0.4 produce `ctrl`+`b` press and release, `ctrl`+`1` press. <br>
Combination `+ctrl+b+1` in 0.4 produce `ctrl`+`b`+`1` press. <br>
As side effect console command syntax changed from: `+~+quit:symbols-short+enter+~+` to  `+~+quit:symbols-short+enter+$$+~+` <br>
Old syntax may still work for you. But it leaves ambiguity for the operating system about what at the end of command.
All prebuild control are updated for this release. <br>
A synthetic key's `$$` introduced that represent end of current combination.
Added support for device other that keyboard, added axial manipulation support. 
Implement detailed log for executed command.

<details>
<summary><b>Detailed explanation</b></summary>

To explain what changed first, we need to explain what a command parser is and how it works before 0.4. <br>
Combination command format: ```[[+key[:key-modifier]+][symbols]]...```
- `+key[:key-modifier]+` also `key` sequence of characters enclosed with `+` symbol
    - `key` also `key-name` name of key or `synthetic-key`
    - `key-modifier` optional: most often it is a type of pressing
- `symbols` any characters that do not match first rule
- `synthetic-key` `key-name` that does not exist on real keyboard

Parser execution algorithm before 0.4:
- if text block contain only `symbols` or `synthetic-key` representing symbols interpreter type it as is, and move to next block.
  - if text block contain `key` that is modifier key (ctrl, alt, shift) it will-be applied to current modifier's, then move to next block without typing.
  - if text block contain `key` that is not modifier key, type it with current modifier applied, then move to next block.
  - if this is end of string, interpretation ended with success.

Examples:
- `+ctrl+1`, `+ctrl+1+` is not same, but output is the same. Apply `ctrl` to modifier's list then type `1`
    - `ctrl` and `1` key pressed
  - `+ctrl+alt+1` Apply `ctrl` to modifier's list, apply `alt` to modifier's list, then type `1`
  - `ctrl`, `alt`, `1` key pressed
  - `+ctrl:long+1` Apply `ctrl` to modifier's list then type `1` as long pressed
      - `ctrl` and `1` key long pressed
  - `+ctrl+b+1` This is where old parser work unexpectedly. Apply `ctrl` to modifier's list then type `b1`
      - `ctrl` and `b` key pressed and released, then `ctrl` and `1` pressed
  - `+~+quit:symbols-short+enter+~+` press and release `~`, fast type `quit`, press and release `enter`, press `~`

Interpreter execution algorithm in 0.4:
- if text block contain only `symbols` or `synthetic-key` representing symbols interpreter type it as is, and move to next block.
  - if text block contain `key`, new combination context with default `key-modifier` created
      - current and each follow `key` will-be pushed to context, combination `key-modifier` will-be reevaluated for each new `key-modifier`, it continues until one of followed happened:
          - end of string, combination will-be executed interpretation ended with success.
          - `symbols` or `synthetic-key` representing symbols found, combination will-be executed as is. Interpretation for this block will continue from the beginning of algorithm. unless:
              - if (only) `symbols` is last block of text and content length equal to 1 then this block is not `symbols` and parent rule does not apply.
          - a `synthetic-key` `$$` found, combination will-be executed as is. Interpretation move to next block and continue from the beginning of algorithm
          - if combination storage overflow (more than 6 keycodes for keyboard), exception will rise
          - if `key-modifier` appear more than once for this combination, interpretation ended with error
          - if key already present in combination, it will not be added again, interpretation continues
  - if this is end of string, interpretation ended with success.

Examples:
- `+alt+f`, `+alt+f+` is same for interpreter
    - `alt` and `f` key pressed
  - `+alt+synthetic-1`, `+alt+synthetic-1+` not same
      - `+alt+synthetic-1` first `alt` will-be pressed and released then `synthetic-1` was typed with normal speed
      - `+alt+synthetic-1+` press `alt` and `synthetic-1` keys or fail
  - `+alt:long+ctrl+del`, `+alt+ctrl:long+del` are valid and same, but first is preferred
      - `alt` and `ctrl` and `del` long pressed
  - `+alt:long+ctrl:long+del`, `+alt:short+ctrl:long+del` are invalid, key-modifier must be present once or not present
  - `+alt:long+$$+del:short` is valid, as it two separate combination separated with `$$`
      - `alt` long pressed and released, then `del` short pressed
  - `+alt+mouse-b0:double+` valid
      - press `alt`, wait ~10ms, press `mouse-left` wait for `double` timeout then release in reverse order, wait for `double-spacer` timeout, repeat once
  - `+alt+mouse-b0` technically valid
      - press `alt` and release then type with normal speed `mouse-b0`
  - `+mouse-b0+alt+` valid but executed with different order
      - press `alt`, wait ~10ms, press `mouse-b0`, wait for `press` timeout then release in reverse order
  - `+mouse-pos(0,0)+$$+mouse-left+`
      - set pointer to coord 0,0 and then `mouse-left` press
  - `+mouse-pos(0,0)+mouse-left+`
      - set pointer to coord 0,0 and `mouse-left` press simultaneously. Mouse coord will-be reported twice, on press and release.
  - `+js-axis0(100):long+$$+mouse-left+`
      - set joystick(0) axis to value 100 (modifier was irrelevant), then press `mouse-left`
  - `+js-axis0(100):long+mouse-left+`
      - set joystick(0) axis to value 100 and long press `mouse-left` simultaneously, joystick(0) will-be reported twice, on press and release.
  - `+~+quit:symbols-short+enter+~+` trike one, while it still work, better to change it to `+~+quit:symbols-short+enter+$$+~+` as:
      - `+~+quit:symbols-short+enter+~+` press `~` and release, fast type `quit`, press `enter` and `~` simultaneously
      - `+~+quit:symbols-short+enter+$$+~+` press `~` and release, fast type `quit`, press `enter` and release, press `~`
</details>

#### Further improvement of the connection flow.
Fixed multiple bug's in connection/reconnection flow. <br>
Fixed session fail to close when time come. <br>
Implemented websocket custom close codes. Which will significantly improve and simplify connection status tracking. <br>
Added message distinguish between messages: server perform connection close(for reason) and a client loss of connection. <br>
Improve behavior when booth server and client begin disconnect sequence (client notify server about it's attempt to leave). <br>
Added workaround for situation where overlay worker late loading reset newly created socket connection. This is done by enforce strict loading order. Before fix that problem led to notification spam
connected/disconected/reconnected. Fix not 100% stable and relatable, and probably be fully fixed later.
For build with `SOCKET_KEEP_ALIVE_TIMEOUT > 0` (disabled by default) added deep sleep detector and recoverer. In order to prevent
on such build repeated reconnection when client go to sleep and not able to send keep_alive packet any longer, forcing server to close connection.

#### Few new controls.
Following controls added to registry:

| `name`             | `type`    | `note`                           | * | `name`           | `type`    | `note`                           |
|--------------------|-----------|----------------------------------|---|------------------|-----------|----------------------------------|
| `Decouple`         | `switch`  | decoupled mode                   | * | `Transform`      | `switch`  | Expand/Retract configuration     |
| `IFCS Gravity`     | `switch`  | IFCS - Gravity Compensation      | * | `IFCS Wind`      | `switch`  | IFCS - Wind Compensation         |
| `IFCS - Proximity` | `switch`  | IFCS proximity assist            | * | `IFCS Stability` | `switch`  | IFCS - Stability                 |
| `IFCS Command`     | `switch`  | IFCS command behaviour           | * | `IFCS Core`      | `switch`  | IFCS - Core                      |
| `Advanced HUD`     | `switch`  | -                                | * | `Autoland`       | `oneshot` | -                                |
| `Docking Camera`   | `switch`  | -                                | * | `Staggered Fire` | `switch`  | -                                |
| `Tracking camera`  | `switch`  | Missiles/Enable Cinematic Camera | * | `Tractor Push`   | `oneshot` | Tractor Beam - Increase Distance |
| `Tractor Pull`     | `oneshot` | Tractor Beam - Decrease Distance | * | -                | -         | -                                |

#### Bug fixes.
Fixed problem for old browser (ff52) when session identifier unable to refresh due `same-site` rule violation, leading to connection termination for security reason. <br>
Fixed problem for old browser (chrome57) for http connection when client identifier fail to generate, due to `only secure origins are allowed` on crypto api access, leading to constant connection fail. <br>
Possible fix: client crash produced by overlay worker load in situation where is no connection. <br>
Fixed touch related crash in configure mode.
Fixed mistype in interpreter for key name: `numenther` instead of `numenter` (breaking changes). <br>
Fixed mistype in interpreter for key name: `numrigth` instead of `numright` (breaking changes). <br>
Fixed bug then limiter configuration window prodice error on edit. <br>
Possible fix: after release 0.3 favicon was missing due to compression applied on it.
  - Keep in mind that the issue has likely been fixed, but the changes will only become visible after a few weeks due to the extremely aggressive favicon caching used by some mobile browsers.
  - If your device not affected by compressed favicon problem, you can compress it to save some bytes on flash by compile with `-DFAVICON_COMPRESSION=ON` and `-DRESOURCE_COMPRESSION=ON` flags set.

Fixed incorrect position and size of text block "done" for select control window.<br>
Fixed rare server crash when it is unable to retrieve ip information about incoming socket connection.<br>
Fixed memo control produce `set` activation combination for every activation type. <br>
Fixed memo network state sync end with error.<br>
Possible fix: widget crash caused by multiple small resize. <br>
Possible fix: widget trail on high resolution. <br>

#### Other changes.
Implemented ability for axial client widget to bind with specific joystick. Note that current implementation limitation prevent
binding widget to more than one joystick.

#### Breaking changes.
Token name for key num-enter changed from `numenther` to `numenter`. <br>
Token name for key num-right changed from `numrigth` to `numright`. <br>
Define `LOG_JOYSTICK` removed. <br>
Define `LOG_KEYBOARD` renamed to `LOG_USB_DEVICE` and now control entire composite usb device. <br>
Console input command syntax changed and require separator symbol `$$`. For example instead of `+~+quit:symbols-short+enter+~+`
should be used `+~+quit:symbols-short+enter+$$+~+`
  - All build-in control are updated, check it if you change they combination keys
  - Old syntax will also work, but it leaves OS place to misinterpret command
  - More info in [Parser/interpreter changes](#command-parserinterpreter-changes)

#### Known issues.
Series of fast rotation combined with transition and app navigation, break layout app on modern chrome browser.
In my tests, the bug was reproducible in 30% of cases. Unfortunately, the problem was discovered too late; the fixes will not be included in this patch.
Workaround: 
 - Disable rotation after select a proper orientation.
 - If bug occur, rotate device few time's, for me it restores layout

## Appeal to the community.
Since patch 4.9, I can't play Star Citizen due to an outdated processor. While this may still be fixed by the CIG,
it won't change the fact that the CPU isn't technically supported. Since I can't afford to upgrade my PC right now,
I'll have to start a donation drive. This is completely voluntary; the app will remain free and open source.

## Donations
#### [Boosty.to](https://boosty.to/alhimik.dev/donate)
<br>
