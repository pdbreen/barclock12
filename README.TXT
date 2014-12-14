WinClock(tm) v1.2 Copyright 1993 by Patrick Breen - All rights reserved.


Archive Contents:

   README.TXT    -  this file
   CHANGES.TXT   -  revision history of WinClock
   WINCLOCK.EXE  -  WinClock executable
   CLOCKDLL.DLL  -  WinClock support DLL


Description:

WinClock is a utility that displays useful information in the caption
bar of the active window.  This approach is somewhat less obtrusive
than having the information in a window which is always on top or
that can be covered by other windows.


System Requirements:

WinClock is completely self-contained and is compatable with both
Windows 3.0 and Windows 3.1.  WinClock uses very little memory (less
than 30k) and uses very few resources.


Installation:

Since you are reading this, I assume that you have unzipped the WinClock
archive.  The rest of the installation is very easy.  Create a new program
manager item for WinClock.  Make sure CLOCKDLL.DLL is in the working
directory or put it in your Windows directory.  Double-click to start
WinClock.  I prefer to add the program item to the Startup group in
Windows 3.1.  This causes WinClock to launch every time I start Windows.


Instructions:

Once WinClock is running, information will appear in the caption bar of the
active window.  Initially, WinClock displays the date on the left of the bar
and the time on the right.  A short click on either the time or the date will
toggle the display of free memory.  Another short click will display the
percentage of free resources available.  One more click will display the
disk space available on drive C.  Click and hold the mouse button to display
a menu.  The menu commands are as follows:

Setup...
	The setup dialog is used to specify how the information is displayed.
	The large group of radio buttons is used to control where each of
	the items appears.  An item can either appear on the left or right
	of the caption bar or it can be toggled into view.  If more than one
	item is specified for the left or right, they are combined and
	displayed together.  If more than one item is set to toggled, the
	first short click displays the first toggled item, a click on this
	item displays the next item, etc.  In addition, an item can be turned
	off which prevents it from being displayed at any time.  (If all items
	are turned off or all are set to toggle, the time will continue to
	display so the menu can be accessed!)

	In addition this dialog is used to set the time format and date format.
	The clock can be 24-hour and optionally display seconds.  The date
	format can be chosen from the set of formats displayed.

About...
	Provides the current version number and tells you where
	you can reach me.

Exit
	Removes WinClock and quits the application.


Advanced Options:

I have added the following advanced options (translation - there is no UI
to set them!) to modify the behavior of WinClock.  All of these options are
controlled by adding (or changing) lines in the WinClock.INI file which
can be found in your Windows directory.

ShowWindow=

This determines whether the WinClock window appears if the clock cannot
find a home.  By default, the window does appear and this value is set
to 1.  To keep the window from appearing, change this value to a 0.

SystemColors=

This determines whether WinClock uses the system colors to draw the
text or a custom color that from this .INI file.  By default, WinClock
uses the system colors and this value is set to 1.  To specify a custom
text color, change this value to a 0 and specify the color by adding the
following lines to the .INI file:
TextColorR=(red component of text color)
TextColorG=(green component of text color)
TextColorB=(blue component of text color)

EraseAlways=

This determines whether WinClock invalidates the desktop window to
remove "ghost" clocks in Windows 3.0.  For machines that are running
Windows 3.0, this option will eliminate the redraw that occurs when
switching between applications.  By default, WinClock does invalidate
and this value is set to 1.  This option has no effect if you are using
Windows 3.1.

LeftOffset=
RightOffset=

These two values specify how much room WinClock should leave between
the string it draws and the standard system controls.  These can be
useful if you are using other utilities that want to put controls in
the title bar.  By default, both values are set to 0.


Shareware:

Feel free to distribute this program to others.  All I ask is that this
file remain with the distributed copies.  This product is shareware.  If
you like it, send $5.  If you want to licence a number of copies, the
following table gives the discount rates:

Number of users	Cost		Price per user
  25				$ 100	$4.00
  50				$ 150	$3.00
 100				$ 250	$2.50
 250				$ 500	$2.00
 500				$ 750	$1.50
1000				$1000	$1.00


Disclaimer:

This software is distributed "as is".  Although I know of no problems
with the software, I make no warranties.


Other:

If you have any problems with this program or ideas for features that
should be added, I can be reached at the following electronic addresses
(in addition to the mail address and telephone number below).

Internet:    pbreen@world.std.com
CompuServe:  70312,743

___________________________ Order Form _________________________________


Send To:

Patrick Breen
3920 Mystic Valley Parkway #1119
Medford, MA 02155

Telephone:   (617) 396-2673


(  )  I enclose $5 for encouragement of continuing development



		  Name: ___________________________ Date:_______________


	    Company: ________________________________________________


         Address: ________________________________________________


City, State, Zip: ________________________________________________


         Country: ________________________________________________


       Day Phone: ___________________  Eve:  _____________________


Electronic Mail address: _________________________________________


How did you hear about WinClock v1.2? ____________________________


Comments:
