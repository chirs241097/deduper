# QDeduper User Manual

QDeduper is a graphical frontend for Deduper. It can scan local images for potential duplicates,
create a database for scanned images, and find images in the database that are similar to any
given image ("reverse image search").

Please note at this point QDeduper is unfinished and considered experimental. Many planned
features are not implemented. Only current, tested features will be covered in this manual.

## Basic Terminology

- Image signature: A small piece of data (usually < 1 KiB) that captures visual features of the
image. By comparing these small signatures we can avoid the cost of comparing images
pixel-by-pixel.

- Distance: Images are compared by computing the "distance" between their signatures. The smaller
the distance, the closer two images will appear visually. There's a threshold distance beyond
which two images are no longer considered similar.

- Signature database: All scanned images and their signatures are kept in a database. This
database is indexed to allow quick lookups.

- Group: Images that are similar will be grouped together, forming these "groups". Images in the
same group don't have to be considered similar to each other. For example if image A and image B
are considered similar, and image B and image C are considered similar, but image A and image C
are not considered similar, all three will be placed in the same group.

## Running QDeduper

There's currently no official binary for QDeduper. You may build it yourself (see README.md of
the project), or grab an automatic CI build from GitHub (Windows only for now).

After obtaining a working build of QDeduper, just launch the single executable inside. QDeduper
currently accepts no arguments.

## User Interface

```
--------------------------------------------------------------------
File  View  Marks  Help
--------------------------------------------------------------------
(Toolbar)
--------------------------------------------------------------------
                                            |
                                            |
                                            |
                                            |
                                            |
                  (1)                       |         (2)
                                            |
                                            |
                                            |
                                            |
                                            |
                                            |
--------------------------------------------------------------------
(Status bar)
--------------------------------------------------------------------
```

(1) : Image list. Similar images in the current group will be listed here.
Each image will be listed with its assigned hotkey, file name, image dimensions, and file size.
Right click on an item in the list to perform actions on it. Available actions are:

 - Mark / Unmark : mark or unmark the image
 - Mark All Except: mark all images except this one.
 - Maximize: enter single image mode and show this image only.
 - Open Image: open the current image with system image viewer.
 - Open Containing Folder: locate the image in file browser.

For details on how image hotkeys work and how to configure them, see the section on Shortcuts tab
in Preference.

(2) : Difference between images: This shows distances between images. Images are represented by
their hotkeys.

(Toolbar) : A fully customizable toolbar. Most of the menu items can be placed here. See the
section on Toolbar tab in Preference.

(Status bar) : Shows information related to the current view. In normal operation it shows the
number of groups found and the current group on the right side.

### Menu Actions

#### File

 - Create Database... : Create a new signature database by scanning a set of folders. Choose the
 folders to scan in the dialog. Press "Browse..." to add folders to scan. "Delete" removes a folder
 from the list. Check the "Recursive?" box if you want all subdirectories within the path to be
 scanned. Hit "Scan" to start the scanning process. The scanning progress can be monitored in the
 progress dialog, and can be cancelled if desired.  
 If the scanning process is cancelled, the created database will be incomplete. There's currently
 no way to resume a cancelled scan.  
 If you have unsaved changes (database or marks), you'll be asked before they will be discarded.

 - Load Database... : Load existing signature database from disk.

 - Save Database... : Save the current database to disk. Please note that markings are saved in a
 separate file (see below).

 - Export Marked Images List... : Save the list of images currently marked. File names of all
 marked images across all groups will be exported to a file. The exported file will have one file
 name on each line. This file can be used to perform batch operations on marked images, e.g.
 moving them to another location.

 - Import Marked Images List... : Import a list of images that should be marked. All current
 markings will be lost.

 - Search for Image... : Select any image on the disk. If the current signature database contains
 images that are visually similar to the selected image, they will be shown in the image list.  
 You cannot mark images in the result image list in this mode. Other image actions still function
 as normal. Use Previous / Next Group or Skip to Group to exit from the result image list.

 - Preference... : See the preference section.

 - Exit : Quit QDeduper. You will be asked to save unsaved changes if there is any.

#### View

 - Next Group : Show next group of similar images.
 - Previous Group : Show previous group of similar images.
 - Skip to Group : Skip to a certain group.
 - Single Image Mode : By default, QDeduper try to fit as many images as possible on screen.
 In single image mode, each image will take up (almost) the entire screen.
 - Sort by : Choose how items in the image view are sorted. You can sort by file size,
 image dimension, file path, or keep the default order.

#### Marks

 - Mark All : Mark all images in the current group.
 - Mark None : Unmark all images in the current group.
 - Mark All within directory... : Choose a directory. All images from that directory **across all
 groups** will be marked.
 - Mark All within directory and subdirectory... : Choose a directory. All images from that
 directory and its subdirectories **across all groups** will be marked.
 - Review Marked Images : Show a list of currently marked images.

#### Help

 - Help : In application help is not implemented. It will redirect you to the document you are
 currently viewing.
 - About : Boilerplate dialog.
 - About Qt : Cute.

## Preferences

### General

 - Minimal Dimension in Image View : By default Deduper will try to fit as many images as possible
 into the image view. It will shrink the images in the view until either side of the image reach
 this size.
 - Number of Threads : Number of concurrent signature computation threads used during the scanning
 process. If 0, a thread will be spawned for each logical processor the computer has. If unsure,
 leave as 0.
 - Show Text in Toolbar Buttons : If unchecked, each action in the toolbar will display as an icon.
 - Show Database Engine Memory Usage : Signature database are stored in system memory. Check this
 to show the amount of memory used by the database in the status bar.

### Signature

 - Distance Threshold : The threshold distance beyond which two images are no longer considered
 similar. Applies only to databases created after the change of this value.

### Shortcuts
 
Most menu actions can be assigned a shortcut key here. To modify the shortcut for an action,
double click the hotkey in its row and type the shortcut. After one second the new key combination
will be recorded.

Menu actions named "Item ?? Action Key" are special actions assigned to items in the image view:
Item 1 Action Key will be assigned to the first item in the view, Item 2 Action Key will be
assigned to the second item and so on. These keys combined with the Action Modifiers below produces
the actual hotkey for an action on an item. For example:

 - If modifier for Mark / Unmark is set to "No modifier" and Item 4 Action Key is set to "F", then
 pressing "F" will mark or unmark the fourth image in the image view.
 - If modifier for Open with System Viewer is set to "Shift" and Item 9 Action Key is set to "L",
 pressing "Shift+L" will open the ninth image in the image view with system image viewer.

Press the button in the action modifiers section to change modifiers assigned to a certain action.
Input the desired modifier together with Enter / Return key, or space bar. If you press escape
instead, "No modifier" will be assigned to this action, meaning pressing the shortcut key assigned
to an item alone triggers this action for that item. If you press Delete or Backspace, the modifier
for this action will be set to "Disabled", which means this action will never be triggered by
hotkeys.

A max number of 16 hotkeys can be assigned to the first 16 images in the image view. If some images
can't be assigned to a hotkey, you will receive a warning.

### Toolbar

The toolbar in the main window is fully configurable. You can drag to change its docking side, or
make it a standalone window. Configure actions shown in the toolbar here.

To add an action to the toolbar, select the action from the available action list on the left and
click the ">" button.

To remove an action from the toolbar, select the action to remove on the right and click the "<"
button.

To sort the actions, drag an action to its desired position on the right side.
