#!/bin/bash
#
# script to fetch pak sets
#

# make sure that non-existing variables are not ignored
set -u

# fall back to "/tmp" if TEMP is not set
TEMP=${TEMP:-/tmp}


# parameter: url and filename
do_download(){
if which curl >/dev/null; then
    curl -L "$1" > "$2" || {
      echo "Error: download of file $2 failed (curl returned $?)" >&2
      rm -f "$2"
      exit 4
    }
else
    if which wget >/dev/null; then
        wget -q -N "$1" -O "$2" || {
          echo "Error: download of file $2 failed (wget returned $?)" >&2
          rm -f "$2"
          exit 4
        }
    else
        echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
        exit 6
    fi
fi
}


# two parameter, url, zipfilename
DownloadInstallZip(){
  echo "downloading from $1"
  do_download "$1" "$TEMP/$2"
  echo "installing from $2"
# first try to extract all files in simutrans/
  unzip -q -o -C "$TEMP/$2" -x "simutrans/*" 2> /dev/null

  if [ $? -lt 2 ]; then
#     no simutrans folder in the zipfile
#     unzip directly into simutrans/
      unzip -o -C -q "$TEMP/$2" -d simutrans
  fi
  rm "$TEMP/$2"
}


# generated list of pak sets
paksets=( \
"https://downloads.sourceforge.net/project/simutrans/pak64/120-4/simupak64-120-4.zip" \
"https://www.simutrans-germany.com/pak.german/pak64.german_0-112-3-10_full.zip" \
"https://downloads.sourceforge.net/project/simutrans/pak64.japan/120-0/simupak64.japan-120-0-1.zip" \
"https://github.com/wa-st/pak-nippon/releases/download/v0.4.0/pak.nippon-v0.4.0.zip" \
"https://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip" \
"https://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20111-3/pak96.comic-0.4.10-plus.zip" \
"https://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20ST%20120.4.1%20%282.8.1%2C%20priority%20signals%20%2B%20bugfix%29/pak128.zip" \
"https://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20120-3/pak128.Britain.1.18-120-3.zip" \
"https://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_1.0_for_ST_120.x/PAK128.german_1.0_for_ST_120.x.zip" \
"https://downloads.sourceforge.net/project/simutrans/pak192.comic/pak192comic%20for%20120-2-2/pak192.comic.0.5.zip" \
"https://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip" \
"https://downloads.sourceforge.net/project/simutrans/pak64.contrast/pak64.Contrast_910.zip" \
"http://hd.simutrans.com/release/PakHD_v04B_100-0.zip" \
"http://pak128.jpn.org/souko/pak128.japan.120.0.cab" \
"https://downloads.sourceforge.net/project/simutrans/pak64.scifi/pak64.scifi_112.x_v0.2.zip" \
"https://downloads.sourceforge.net/project/ironsimu/pak48.Excentrique/v018/pak48-excentrique_v018.zip" \
)

tgzpaksets=( \
 "http://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz" \
)

choices=()
installpak=()

echo "-- Choose at least one of these paks --"

let setcount=0
let choicecount=0
let "maxcount = ${#paksets[*]}"
while [ "$setcount" -lt "$maxcount" ]; do
      installpak[choicecount]=0
      urlname=${paksets[$setcount]}
      zipname="${urlname##http*\/}"
      choicename="${zipname%.zip}"
      choicename="${choicename/simupak/pak}"
      choices[choicecount]=$choicename
      let "setcount += 1"
      let "choicecount += 1"
      echo "${choicecount}) ${choicename}"
done

while true; do
  read -p "Which paks to install? (enter number or (i) to install or (x) to exit)" pak
#exit?
  if [[ $pak = [xX] ]]; then
    exit
  fi
# test if installation now
  if [[ $pak = [iI] ]]; then
    echo "You will install now"
    let setcount=0
    while [ $setcount -lt $choicecount ]; do
      if [ ${installpak[$setcount]} -gt 0 ]; then
        let "displaycount=$setcount+1"
        echo "${displaycount}) ${choices[$setcount]}"
      fi
      let "setcount += 1"
    done
    read -p "Is this correct? (y/n)" yn
    if [[ $yn = [yY] ]]; then
      break
    fi
    # edit again
    echo "-- Choose again one of these paks --"
    let setcount=0
    while [ $setcount -lt $choicecount ]; do
      echo "${setcount}) ${choices[$setcount]}"
      let "setcount += 1"
    done
    let "pak=0"
  fi
# otherwise it should be a number
  if [[ $pak =~ ^[0-9]+$ ]]; then
	let "setcount=pak-1"
	if [ $setcount -lt $choicecount ]; then
	if [ $setcount -ge 0 ]; then
		status=${installpak[$setcount]}
		if [ $status -lt 1 ]; then
		echo "adding ${choices[$setcount]}"
		installpak[$setcount]=1
		else
		echo "not installing ${choices[$setcount]}"
		installpak[$setcount]=0
		fi
	fi
	fi
  fi
done

# first the regular pak sets
pushd ..
let setcount=0
let "maxcount = ${#paksets[*]}"
while [ "$setcount" -lt "$maxcount" ]; do
  if [ "${installpak[$setcount]}" -gt 0 ]; then
    urlname=${paksets[$setcount]}
    zipname="${urlname##http*\/}"
    DownloadInstallZip "$urlname" "$zipname"
  fi
  let "setcount += 1"
done
popd
exit
