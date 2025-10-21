# Soir

Soir is a groovebox built for Nintendo 3DS (particularly New Nintendo 3DS) using devkitpro and libctru.

## Build instructions

### Building the 3DS app

You will need to setup a dev environment with devkitPro and grab all the 3ds related libs and the devkitpro version of opusfile

To build:

```make```

### Building Unit tests

This is currently setup to use clang (my main dev machine is a MBP) so some fiddling might be needed if you're on a different platform

To test the 3DS independent code from your dev computer:

```make test```

### Other make commands available

To format code:

```make format```

To clean the 3DS build:

```make clean```

To clean the tests build:

```make test-clean```
