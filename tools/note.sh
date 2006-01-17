#!/bin/sh
#
# USAGE:
#  From the top source directory, run ./tools/note.sh
#  It will open your editor, with a template of a CVS ChangeLog Note,
#  or open a previously edited Note.  Enter your changes, one per
#  line (do *NOT* wrap lines, or let your editor wrap lines).  When
#  you are ready to commit, run ./tools/commit.sh (which will
#  also call this script one last time before committing).

if [ -f ~/.self.info ] ; then
  . ~/.self.info
fi

if [ -z "$FULLNAME" ] ; then
  FULLNAME=$(getent passwd $USER | cut -d : -f 5 | cut -d , -f 1)

  if [ -z "$FULLNAME" ] ; then
    FULLNAME="$USER"
  fi
fi

if [ -z "$EMAIL" ] ; then
  DOMAIN=$(hostname -d)
  if [ -z "$DOMAIN" ] ; then
    DOMAIN=$(hostname)
    if [ -z "$DOMAIN" ] ; then
      DOMAIN=localhost
    fi
  fi

  EMAIL="$USER@$DOMAIN"
fi

if [ -z "$EDITOR" ] ; then
  for EDITOR in `which editor 2>/dev/null` `which vim 2>/dev/null` `which vi 2>/dev/null` ; do
    break
  done
fi

if [ ! -f .commit.log ] ; then
  echo "# Commit By: $FULLNAME <$EMAIL>" >> .commit.log
  echo '# Put one change on each line.  Do not try to wrap' >> .commit.log
  echo '# Long lines; the script will do that for you.  Be' >> .commit.log
  echo '# careful your editor does not try to wrap them for' >> .commit.log
  echo '# you.  Also, any line starting with a # will be' >> .commit.log
  echo '# ignored by the script.' >> .commit.log
  echo 'Summary: ' >> .commit.log
  echo >> .commit.log
fi

#has blank line already?
if expr $(tail -n 1 .commit.log | wc -c) '>' 1 >/dev/null 2>&1 ; then
  echo >> .commit.log # blank line
fi
LINES=$(wc -l < .commit.log)
$EDITOR .commit.log "+$LINES"
