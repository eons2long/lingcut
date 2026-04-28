# macOS Release

This is the current unsigned macOS release flow for LingCut.

## Build an unsigned DMG

Use the existing Shotcut build script from a clean checkout on macOS:

```bash
./scripts/build-shotcut.sh -v 0.1.0
```

The macOS packaging defaults now produce:

```text
LingCut.app
lingcut-macos-0.1.0-unsigned.dmg
```

The unsigned DMG is suitable for internal testing. macOS Gatekeeper will warn
users because the app is not signed and notarized.

## Upload to GitHub Releases

After verifying the DMG locally:

```bash
git tag -a v0.1.0 -m "LingCut 0.1.0"
git push origin v0.1.0
gh release create v0.1.0 \
  /path/to/lingcut-macos-0.1.0-unsigned.dmg \
  --title "LingCut 0.1.0" \
  --notes "Unsigned macOS test release."
```

## Signed Releases

For public macOS releases, use Developer ID signing and Apple notarization before
uploading the DMG. `scripts/codesign_and_notarize.sh` now accepts LingCut names
by default, but the signing identity and Apple notary credentials still need to
be configured for this project.
