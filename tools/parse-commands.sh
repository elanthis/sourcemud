#!/bin/sh

(
echo "// AUTOMATICALLY GENERATED - DO NOT EDIT"
echo "// Generated on $(date)"

for file in src/cmd/*.cc; do
	block=$(sed -n '/BEGIN COMMAND/,/END COMMAND/ p' "$file")

	name=$(echo "$block" | sed -n '/name:/ p' | sed 's/[^:]*: \(.*\)/\1/')
	cmd=$(echo "$name" | sed 's/[^a-z]/_/g')
	if [ "x$name" = "x" ]; then
		echo "ERROR: missing command info block: $file" >&2
		continue
	fi
	echo "// $file"
	echo "COMMAND(\"$name\","

	usage=$(echo "$block" | sed -n '/usage:/ p' | sed 's/[^:]*: \(.*\)/\1/')
	if [ "x$usage" != "x" ]; then
		echo "$usage" | while IFS= read usage; do
			echo "\"$usage\n\""
		done
		echo ","
	else
		echo "\"$name\n\","
	fi

	echo "command_$cmd,"

	access=$(echo "$block" | sed -n '/access:/ p' | sed 's/[^:]*: \(.*\)/\1/')
	if [ "x$access" = "x" ]; then
		access=ALL
	fi
	echo "ACCESS_$access,"

	if grep -q "^void command_$cmd *(Player" "$file"; then
		echo "Player)"
	else
		echo "Creature)"
	fi

	format=$(echo "$block" | sed -n '/format:/ p' | sed 's/[^:]*: \(.*\)/\1/')
	if [ "x$usage" != "x" ]; then
		echo "$format" | while IFS= read format; do
			priority=$(echo "$format" | sed -n '/([0-9]\+)/ p' | sed 's/.*(\([0-9]\+\))$/\1/')
			if [ "x$priority" = "x" ]; then
				priority=50
			fi
			format=$(echo "$format" | sed 's/\(.*\) ([0-9]\+)$/\1/')
			echo "FORMAT($priority, \"$format\")"
		done
	else
		echo "FORMAT(50, \"$name\")"
	fi

	echo "END_COMM"
done

) >src/cmd/commands.h || exit 1
