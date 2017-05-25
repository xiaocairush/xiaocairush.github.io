---
layout: post
title:  "Vim Learning Notes"
date:   2017-05-25 22:00:00 +0800
categories: vim
---

This is a learning notes of  [《vimbook-OPL.pdf》](https://docs.google.com/viewer?a=v&pid=sites&srcid=ZGVmYXVsdGRvbWFpbnxrb25nbHR8Z3g6NTUyOGM0NmRlYmQzY2E5Mg)

## Basic Command:

| command | description |
|---------|-------------|
| :help | get help. You can also use \<F1\>. |
| h,j,k,l | move cursor to left,down,up,right |
| 2h | an example to move cursor to left 2 character. use \<n\>h/j/k/l to move faster |
| o | open new line below cursor |
| O | open new line above cursor |
| a | append |
| 3a5\<Esc\> | an example to append '555'. similarly, command 3x will delete 3 characters. |
| i | insert |
| ZZ | save and close |
| ctrl+] | jump to tag |
| ctrl+T | pop a tag off the tag stack. (go back to location before jump tag.) |
| dd | cut line into vim clipboard. |
| dw | cut a word |
| d3w | cut three words |

## Edit A Little Faster

| command | description |
|---------|--------------------------|
| w | move cursor forward one word |
| b | move cursor back one word |
| 2w | move cursor forward two word |
| $, \<End\> | move cursor to end of line |
| ^ | move cursor to first nonblank character of the line |
| \<Home\> | move cursor to first character of the line |
| 2$ | move cursor to end of next line |
| \<kHome\> | move cursor to first character of current line |
| f\<x\> | forward search character \<x\>.|
| 5f\<x\> | forward move cursor to fifth character \<x\> of current line |
| F\<x\> | back search character \<x\>. |
| t\<x\> | forward search character \<x\> but move cursor before the character \<x\> |
| T\<x\> | back search character \<x\> but move cursor after the character \<x\> |
| 3G | move cursor to the 3rd line |
| :set nu | show line number |
| :set nonu | don't show line number |
| ctrl+G | displays a status line that indicates where you are in the file |
| ctrl+U | scrolls up half a screen of text |
| ctrl+D | scrolls down half a screen of text |
| d$, D | delete to end of line |
| cw | change a word |
| cc | change the whole line |
| . | repeats the last delete or change command |
| J | joins the current line with the next one |
| r\<x\> | replaces the character under the cursor with \<x\> |
| q\<x\> | Start recording a macro in register x |
| q | Stop recording the macro |
| :digraphs | show available digraph symbols |

## Searching
Special characters: *[]ˆ%/\?~$ has special meaning.

| command | description |
|---------|-------------|
| /\<string\> | search \<string\> |
| /\<Enter\>, n | find next result |
| N | find last result |
| /\<Up\> | last search history |
| /\<Down\> | next search history |
| :set hlsearch | highlight any strings found matching the search pattern|
| :set nohlsearch | turn off search highlighting |
| :nohlsearch | clear the current highlighting |
| :set incsearch | turn on incremental searches |
| :set noincsearch | turn off incremental searches |
| ? | searches backward. ? and n commands work together |

## Text Blocks and Multiple Files

| command | description |
|---------|-------------|
| p | paste the text before the cursor |
| m\<a\> | marks the place under the cursor as mark \<a\>|
| `\<a\> | go to mark \<a\> |
| :marks | list all marks |
| yy, Y | yanks the current line into the buffer (copy operation)|
| y`\<a\> | copy text between current cursor and mark \<a\> |
| \<n\>Y | yank n lines into the registers |
| !\<motion\> <br/> \<command\> | It runs the system command represented by command,<br/> giving it the block of text represented by motion as input.<br/> The output of this command then replaces the selected block.|
| !10Gsort | The result is that the sort program is run on the first 10 lines.<br/> The output of the programreplaces these lines.|
| !!ls | This puts the output of the ls command into current line. |
| !!date | This puts the output of the date command into current line.|
|| **For !!ls and !!date Using !! like this is technically not filtering <br/> because commands like ls and date don’t read standard input.** |
| :vi file | edit another file|
|| **The :e command can be used in place of :vi**|
| :write | save current file |
| :vi! file.txt | force Vim to discard your changes and edit the new file using the force (!) option|
| :view file | the new file is opened in read-only mode |
| vim \<file1\> \<file2\> \<...\> | edit multiple files. By default, Vim displays just the first file.To edit the next file, you need to change files using the :next command|
| :next | edit the next file |
| :wnext | save file and edit the next file |
| :next! | edit the next file |
| :set autowrite | turn on auto write. |
| :set noautowrite | turn off auto write |
| :2 next | excute :next twice |
| :args | displays the list of the files currently being edited |
| :previous :Next| edit the previous file |
| :wprevious | save file and edit the previous file |
| :wNext | save file and edit the previous file |
| :rewind :first | edit the first file |
| :last | edit the last file |
| ctrl+^ | switch editing from the current file to the alternate file|
| \<n\>ctrl+^ | switch to \<n\>th file |

## Window && Buffer

| command | description |
|---------|-------------|
| :split | open a new window. You can view two different parts of a file simultaneously.|
| ctrl+ww | switch window |
| ctrl+wj | go down a window |
| ctrl+wk | go up a window |
| ctrl+wc \<br/\> ZZ \<br/\>:q | close current window |
| :split \<file\> | open a new window and edit given \<file\> |
| :split +/printf three.c || 
|:3 split \<file\> <br/> :3split \<file\>| Controlling Window Size.<br/> opens a new window three lines high.|
| :count split +command file | count:The size of the new window in lines. (Default is to split the current window into two equal sizes.)<br/> +command: An initial command.<br/> file: The name of the file to edit. (Default is the current file.)
||**This is a summary for split**|
| :new | open a new window and edit new file.<br/> The :new command works just like the :split command.|
| :sview | The :sview command acts like a combination of :split and :view |
| ctrl w+ | increases the window size by count (default = 1). |
| ctrl w- | decreases the window’s size by count (default = 1). |
| \<n\> ctrl w | makes the current window n lines high |
| :hide | hide a window. ( But Vim still knows that you are editing this buffer) |
| :buffers | find a list of buffers|
|| explanation for buffer:<br/>**The first column is the buffer number.<br/> The second is a series of flags indicating the state of the buffer.<br/> The third is the name of the file associated with the buffer.<br/> The state flags are as follows:<br/> - Inactive buffer.<br/> h Buffer is hidden.<br/> % Current buffer.<br/> # Alternate buffer.<br/> + File has been modified.**|
| :buffer number | Selecting a Buffer by number |
| :buffer file | Selecting a Buffer by file name |
| :sbuffer number | splits the window and starts editing the buffer |
| :bnext | Go to the next buffer. |
| :count bnext | Go to the next buffer count times. |
| :count sbnext | Shorthand for :split followed by :count bnext. |
| :count bprevious <br/> :count bNext| Go to previous buffer. If a count is specified, go to the count previous buffer.|
| :count sbprevious<br/> :count sbNext| Shorthand for :split and :count bprevious |
| :blast | Go to the last buffer in the list. |
| :sblast | Shorthand for :split and :blast. |
| :brewind | Go to the first buffer in the list. |
| :sbrewind | Shorthand for :split and :rewind. |
| :bmodified count | Go to count modified buffer on the list. |
| :sbmodified count | Shorthand for :split and :bmodified. |
| :set hidden | If the option hidden is set, files that leave the screen do not become inactive; instead they automatically become hidden.|

## Basic Visual Mode

| command | description|
| ------- | ---------- |
| v | starts a character-bycharacter visual mode.|
| V | starts linewise visual mode. |
| ctrl V | block visual mode |
| :help v_d | help on the commands that operate in visual mode |
| \<Esc\> <br/> ctrl c| Leaving Visual Mode |
| > | indents the selected lines by one “shift width" |
| < | reverse to > |
| ctrl V then Istring\<Esc\>| inserts the text on each line starting at the left side of the visual block|
| ctrl V then c \<Esc\> | change each line in the visual block |
| ctrl V then r\<character\> \<Esc\> | replace the selected characters with a single character |
| ctrl V then > \<Esc\> | The command > shifts the text to the right one shift width <br/>The < command removes one shift width of whitespace at the left side of the block|
| :help v_b_r | Help r command in Visual Block Mode |

## Commands for Programmers

| command | description|
| --- | --- |
| :syntax on | turns on syntax coloring |
| :set filetype=c | tells Vim which type of syntax highlighting to use|
| >> | shifts the current line one shift width to the right |
| << | shifts the current line one shift width to the left |
| :set shiftwidth=4 | set shiftwidth |
| 5<< | shifts 5 lines |
| <motion | shifts each line from the current cursor location to where motion carries you.|
| =motion | indents the selected text using Vim’s internal formatting program. |
| [CTRL-I <br/> ]CTRL-I| Search for a word under the cursor in the current file and any brought in by #include directives.<br/> The [CTRL-I command jumps to the word under the cursor. The search starts at the<br/> beginning of the file and also searches files brought in by #include directives.<br/> The ]CTRL-I does the same thing, starting at the cursor location.<br/>|
| **gd**<br/>**gD**| Search for the definition of a variable.<br/> gd for local variable and gD for global variable. |
| **]CTRL-D**<br/>**[CTRL-D**| Jump to a macro definition.<br/>The [CTRL-D command searches for the first definition of the macro<br/>The ]CTRL-D command searches for the next definition of the macro<br/>|
| **]d**<br/> **[d**<br/> **]D**<br/> **[D**<br/>| Display macro definitions.<br/> The [d command displays the first definition of the macro whose name is under the cursor.<br/> The ]d command does the same thing only it starts looking from the current cursor position and finds the next definition.<br/>The ]D and [D commands list all the definitions of a macro.<br/> |
| **%** | The % command is designed to match pairs of (), {}, or [], /*, #ifndef, #if |
| **>%** | Shifting a Block of Text Enclosed in {} |
| 1\. Position the cursor on the first {.<br/> 2\. Execute the command >i{.<br/> | This shift right command (>) shifts the selected text to the right one shift width. In this case, the selection command that follows is i{, which is the “inner {} block” command.|
| 1. Position the cursor on the left or right curly brace.<br/> 2. Start visual mode with the v command.<br/> 3. Select the inner {} block with the command i}.<br/>4. Indent the text with >.| indent a block using visual mode |
| K | The K command runs a UNIX man command using the word under the cursor as a subject.|
| :tag function | go to a function definition |
| :tags | The :tags command shows the list of the tags that you have traversed through |
| CTRL-T | The CTRL-T command goes the preceding tag. |
| :<count>tag | jump forward that many tags on the list |
| :stag tag | You can split the window using the :split command followed by the :tag command. |
| :tag /\<regular expression\>| Finding a Procedure When You Only Know Part of the Name|

pocessing: 82
