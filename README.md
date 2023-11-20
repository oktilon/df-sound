# Defigo Samantha: Sound Service

## Clone project

```bash
    git clone git@bitbucket.org:d3f1g0/sound.git
```

## Install dependencies

### Developer dependencies (build + run)

```bash
    sudo apt install libsystemd-dev libsqlite3-dev libevent-dev libfreetype-dev libgtk-3-dev libglib2.0-dev libcurl4-gnutls-dev libpango1.0-dev libcairo2-dev
```
> Yocto emakOS already has all this dependencies

## Build project for Developer PC

### Configure developer build
```bash
    cd sound
    meson setup build -D user=$USER
```

### Developer build
```bash
    cd sound
    ninja -C build install
```

### Run locally
```bash
    sudo build/sound -vvv
```

## Build project for Yocto EmakOS

### Configure test build
```bash
    cd sound
    . /opt/emakOS/1.0/environment-setup-cortexa35-poky-linux
    meson dist
```

### Test build
```bash
    ninja -C dist
```

### Copy to Emak
```bash
    scp dist/sound root@10.0.0.124:/home/root/
```
> Where 10.0.0.124 is Emak IP

### Run on Emak
```bash
    ./sound -vvv
```

### Remote debug on Emak
```bash
    gdbserver 0.0.0.0:10000 ./sound -vvv
```

### Remote debug from Host (or use IDE)
```bash
    gdb-multiarch
    (gdb) target remote 10.0.0.124 10000
```
> Where 10.0.0.124 is Emak IP

## Command line options
```bash
    ./sound [-q|-v|-vv|-vvv] [-w]
    where
    -q, --quiet         Log level ERROR only
    -v, --verbose       Log level INFO
    -vv                 Log level TRACE (double --verbose)
    -vvv                Log level DEBUG (triple --verbose)
    -w, --wayland-debug Enable Wayland debug messages
```