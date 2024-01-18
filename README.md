### Cloning

```sh
git clone http://repo.mzjtechnology.com/wiphone/wiphone-firmware WiPhone
cd WiPhone
git submodule init
git submodule update
```

### Compiling

Open the file `WiPhone.ino` in the source folder with Arduino IDE and compile.




### Make a New Release

We follow a development model where:
 - the master branch contains releases (and only releases)
 - the development branch contains release candidates and any work needed to merge in feature branches
 - feature branches contain work related to specific code improvements

The release process starts with a release candidate. Release candidates can be built using the firmware builder tool. Releases are hosted at https://wiphone.io/site/static/releases/firmware/testing/. Once testing is done the release candidate code will be merged into the master branch and released at https://wiphone.io/site/static/releases/firmware/.

Build and code release process:

1. run the code formatter and confirm there are no changes: `astyle --options=./formatter.conf "*.h, *.c, *.cpp, *.ino"`
2. update CHANGELOG.txt
3. increment FIRMWARE_VERSION in config.h appropriately and add an rc number
4. define WIPHONE_PRODUCTION
5. commit changes
6. download or create a .zip file of the current repo
7. upload that to the build tool in the DCS: MZJ/001-009/Tools/wiphone-firmware-release(-candidate)
8. put the 3 resulting files (zipped build directory, .ini file, and firmware binary) on the server (you will need access to the Syncthing share folder). This will be https://wiphone.io/site/static/releases/firmware/ (or firmware/testing/ for release candidates)
9. the zipped build directory is for later reference (includes the commit version of the Arduino SDK builder library, the sdkconfig, and the .elf file so we can do stack traces if needed)
10. on the server, update the soft link for the latest firmware URL to point to the new ini file (or ask Ben to do this)
11. do whatever testing we need to verify the release is good

to make a production build instead of a release candidate:

 - delete the rc number from FIRMWARE_VERSION
 - commit to the master branch
 - add a tag `git tag vX.X.X`
 - build/release the production code similar to above
 - for production releases, first make a link using a different name, check it works, then rename it to WiPhone-phone.ini
 - overwrite the files in the public repo with the latest ones from the master branch and commit those inside the public version of the repo

copy the release files to the WiPhone Tester folder at: WiPhone Engineering/03 - software/WiPhone_Tester/bin/production