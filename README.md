# Historical notice

This project is mostly defunct, but I would like to keep a record of it since it was one of my very first open source projects back in 2003!
It was very relevant when I ran my own mailserver and checked mail using [mutt](http://www.mutt.org/) directly on that mailserver.

# Maildirtree

Maildirtree is a simple program designed to look like [tree(1)](https://linux.die.net/man/1/tree).
It displays hierarchies from a flat Maildir structure in the Courier style (that is, subfolders are named .Folder.Subfolder.)
It displays a full count of messages, the number of unread messages and can be told to only provide a summary of the statistics thereof.

Currently, it ONLY supports Courier-style Maildir hierarchies; recursion and the tree format will not work quite right for other types of hierarchies.

Maildirtree is written in about 500 lines of C and was the object of an autoconf experiment. Therefore, the build process is somewhat overkill for such a small program, but it works nonetheless.

Some example output from Maildirtree (abridged from my own Maildir):

```
Maildir                                 (0/20)
|-- Administrivia                       (0/15)
|-- DebianBugs                          (0/459)
|-- Drafts                              (0/0)
|-- Family                              (0/49)
|-- Friends                             (0/8)
|-- Junk Mail                           (109/109)
|-- Linux                               (0/0)
|   |-- [debian-d-a]                    (0/4)
|   |-- [debian-devel-changes]          (2/2)
|   |-- [debian-devel]                  (0/521)
|   |-- [debian-mentors]                (2/68)
|   |-- [hybrid]                        (0/4)
|   |-- [ircd-coders]                   (0/7)
|   |-- [linux-kernel]                  (0/540)
|   |-- [linux-mm]                      (0/998)
|   `-- [sparclinux]                    (0/426)
|-- Sent Items                          (0/626)
|-- Stanford                            (0/72)
|-- Trash Can                           (0/0)
`-- Archive                             (0/0)

113 messages unread in 3 folders, 4808 messages total.
```

And in its 'summary' mode:

```
Maildir: 37 messages unread in 8 folders, 4003 messages total.
Unread messages in: Maildir, Linux/[d-i-cvs], Linux/[debian-d-a], Linux/[ircd-coders],
Linux/[irssi-users], Linux/[sparclinux], Linux/[hostap], Spam
```

Maildirtree is software released under the terms of the [GNU General Public License, version 2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).

# Feedback
I welcome any feedback to Maildirtree by email: joshk@triplehelix.org.
