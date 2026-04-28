#!/bin/sh
VERSION="$1"
: "${VERSION:?Usage: $0 version}"
sudo xcode-select -s /Applications/Xcode.app/

APP_NAME="${APP_NAME:-LingCut}"
APP_BUNDLE="${APP_BUNDLE:-${APP_NAME}.app}"
APP_PATH="${APP_PATH:-$HOME/Desktop/$APP_BUNDLE}"
DMG_VOLUME_NAME="${DMG_VOLUME_NAME:-$APP_NAME}"
DMG_BASENAME="${DMG_BASENAME:-lingcut-macos-${VERSION}}"
DMG_PATH="$HOME/Desktop/${DMG_BASENAME}.dmg"
TEMP_DMG_PATH="$HOME/Desktop/${DMG_BASENAME}-temp.dmg"
SIGNER="${SIGNER:-Developer ID Application: Meltytech, LLC (Y6RX44QG2G)}"

find "$APP_PATH" -type d -name __pycache__ -exec rm -r {} \+
find "$APP_PATH/Contents" \( -name '*.o' -or -name '*.a' -or -name '*.dSYM' \) -exec rm -rf {} \;
xattr -cr "$APP_PATH"

# Strip any pre-existing signatures so we can overwrite (Qt SDK, etc.)
find "$APP_PATH/Contents" -type f \( -name '*.dylib' -o -name '*.so' \) -exec \
  codesign --remove-signature {} \; 2>/dev/null || true
find "$APP_PATH/Contents" -type d -name '*.framework' -exec \
  codesign --remove-signature {} \; 2>/dev/null || true
find "$APP_PATH/Contents/MacOS" -type f -exec \
  codesign --remove-signature {} \; 2>/dev/null || true

# Re-sign all dylibs and plugins
find "$APP_PATH/Contents" -type f \( -name '*.dylib' -o -name '*.so' \) -exec \
  codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
    --preserve-metadata=identifier,entitlements \
    {} \;

# Re-sign executables with entitlements
find "$APP_PATH/Contents/MacOS" -type f -exec \
  codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
    --preserve-metadata=identifier,entitlements \
    --entitlements ./notarization.entitlements \
    {} \;

# Re-sign the app bundle last
codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
  --preserve-metadata=identifier,entitlements \
  --entitlements ./notarization.entitlements --generate-entitlement-der \
  "$APP_PATH"

codesign --verify --deep --strict --verbose=4 "$APP_PATH"
spctl -a -t exec -vv "$APP_PATH"

# Create DMG with custom background and layout
TMP=$(mktemp -d)
DMGDIR="$TMP/dmg"
mkdir -p "$DMGDIR/.background"

# Move app and create Applications symlink
mv "$APP_PATH" "$DMGDIR/"
ln -s /Applications "$DMGDIR/Applications"

# Copy background image
cp ../packaging/macos/dmg-background.png "$DMGDIR/.background/"

# Create initial DMG (writable)
rm -f "$DMG_PATH"
rm -f "$TEMP_DMG_PATH"
hdiutil create -srcfolder "$DMGDIR" -volname "$DMG_VOLUME_NAME" -format UDRW -size 1500m -fs HFS+ \
  "$TEMP_DMG_PATH"

# Mount the temporary DMG
device=$(hdiutil attach -readwrite -noverify "$TEMP_DMG_PATH" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

# Wait for mount
sleep 2

# Run AppleScript to set up the DMG window
osascript > /dev/null <<EOF
tell application "Finder"
    tell disk "$DMG_VOLUME_NAME"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 1000, 500}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        set background picture of viewOptions to file ".background:dmg-background.png"
        set position of item "$APP_BUNDLE" of container window to {150, 180}
        set position of item "Applications" of container window to {450, 180}
        close
        open
        update without registering applications
        delay 2
    end tell
end tell
EOF

# Unmount the DMG
hdiutil detach "${device}"
sleep 2

# Convert to compressed read-only DMG
hdiutil convert "$TEMP_DMG_PATH" \
  -format UDBZ -o "$DMG_PATH"

# Clean up
rm -f "$TEMP_DMG_PATH"
rm -rf "$TMP"

./notarize.sh "$DMG_PATH"
./staple.sh "$DMG_PATH"

echo Now run:
echo sudo xcode-select -s /Library/Developer/CommandLineTools
