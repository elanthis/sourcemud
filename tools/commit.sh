#!/bin/sh
#
# USAGE:
#  When you are ready to commit, run this script from the top source
#  directory, as ./tools/commit.sh.  ./tools/note.sh will be
#  called (see that file for more information).  Then, your CVS note
#  file will be converted into a ChangeLog entry.  Finally, cvs commit
#  will be called.

# do changelog
CHANGELOG="yes"

# update version
VERSION="yes"

# do push
PUSH="yes"

# remove data dir if asked
while [ -n "$1" ] ; do
  case "$1" in
    -c)
      CHANGELOG="no"
      ;;
    -p)
      PUSH="no"
      ;;
    -v)
      VERSION="no"
      ;;
  esac
  shift
done

# update version
if [ "x$VERSION" = 'xyes' ] ; then
  ./tools/new-version.sh
fi

# load settings
if [ -f ~/.self.info ] ; then
  . ~/.self.info
fi

# guess at full name
if [ -z "$FULLNAME" ] ; then
  FULLNAME=$(getent passwd $USER | cut -d : -f 5 | cut -d , -f 1)

  if [ -z "$FULLNAME" ] ; then
    FULLNAME="$USER"
  fi
fi

# guess at email domain
DOMAIN=$(hostname -d)
if [ -z "$DOMAIN" ] ; then
  DOMAIN=$(hostname)
  if [ -z "$DOMAIN" ] ; then
    DOMAIN=localhost
  fi
fi

# guess at email
if [ -z "$EMAIL" ] ; then
  EMAIL="$USER@$DOMAIN"
fi

export GIT_AUTHOR_NAME="$FULLNAME"
export GIT_AUTHOR_EMAIL="$EMAIL"

# find fmt command
FMT=$(which fmt)

# -u works?
if echo "hi" | $FMT -u >/dev/null 2>&1 ; then
  FMT_U=-u
else
  FMT_U=
fi

# our path
MYPATH=$(dirname "$0")

# run note.sh before commit
"$MYPATH/note.sh"

# begin log output
echo "$(date -u +'%F %T')  $FULLNAME <$EMAIL>" > .commit.new
grep -v -E '^#' .commit.log > .commit.tmp

[ "x$CHANGELOG" = "xyes" ] && echo '' >> .commit.new

# read log input, format output
exec < .commit.tmp
EMPTY="Y"
while read LINE ; do
  if ! echo "$LINE" | grep -q -E '(^#|^Summary:)' ; then
    if [ -n "$LINE" ] ; then
      EMPTY="N"
      if [ "x$CHANGELOG" = "xyes" ] ; then
        if [ -n "$FMT" ] ; then
          echo "$LINE" | fmt -s -w 65 $FMT_U | sed -e '1s/^/  * /;1!s/^/    /' >> .commit.new
        else
          echo "  * $LINE" >> .commit.new
        fi
      fi
    fi
  fi
done

# changelog finish if we are not empty
if [ "x$CHANGELOG" = "xyes" ] ; then
  if test "$EMPTY" != "Y" ; then
    # finish changelog
    echo '' >> .commit.new
    cat ChangeLog >> .commit.new

    # prepare changes
    mv -f ChangeLog .ChangeLog~
    mv -f .commit.new ChangeLog
  else
    cp -f ChangeLog .ChangeLog~
    rm -f .commit.new
  fi
fi

if ! cg-commit -m - < .commit.tmp  ; then
  # failed - revert changes
  [ "x$CHANGELOG" = "xyes" ] && mv -f .ChangeLog~ ChangeLog
  exit 1
fi

mv -f .commit.log .commit.log~

[ "x$PUSH" = "xyes"] && cg-push
