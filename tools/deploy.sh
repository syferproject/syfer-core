#!/bin/bash

GIT_URL="https://github.com/SyferNetwork/syfer-core"
BIN_DIR="$HOME/.local/src/syfer-core/build/src"
WALLETBIN="syferwallet"
BRANCH="master"

INSTALL=0
UPDATE=0

DISTRO="$(awk -F\" '/PRETTY_NAME/ {print $2}' /etc/os-release)"

# Check to see if this is a Raspberry Pi
[ -f "/etc/apt/sources.list.d/raspi.list" ] && isRPI=1 || isRPI=0

# Number of Cores to dedicate to 'make' (make -j$JOBS)
[ "$isRPI" = "1" ] && JOBS=1 || JOBS="$(( $(nproc) / 3))"

## FUNCTIONS ##

# Install Dependencies
installDeps() {
	case $DISTRO in
	"Arch"*|"Artix"*|"Manjaro"*)
		PKGMAN_CMD="sudo pacman -Sy --noconfirm"
		PACKAGES="base-devel git cmake boost boost-libs";;

	*"buntu"*|"Debian"*|*"Mint"*)
		PKGMAN_CMD="sudo apt install -y"
		PACKAGES="build-essential git cmake libboost-all-dev python3-dev";;
	esac

	echo "Installing $(echo $PACKAGES | wc -w) Packages"
	echo "You may be asked to enter your password"

	sudo $PKGMAN_CMD $PACKAGES
}

# Get syfer-core
getSyfer() {
	[ ! -d "$HOME/.local/src" ] && mkdir -p "$HOME/.local/src"

	git clone $GIT_URL -b $BRANCH $HOME/.local/src/syfer-core && \
		mkdir $HOME/.local/src/syfer-core/build || exit
	buildSyfer
}

buildSyfer() {
	cd $HOME/.local/src/syfer-core/build

	cmake .. > buildlog 3>&1 1>&2 2>&3 3>&- && make -j$JOBS >> buildLog 3>&1 1>&2 2>&3 3>&- || buildFail

	which syferd > /dev/null 2>&1 || addToPath
	echo "Build Completed at $(date +'%-I:%M%p on %A %-d %B %Y')"
	exit 0
}

# The build failed
buildFail() {
	printf "An error occured when building - check $HOME/.local/src/syfer-core/build/buildLog for information.\n"
	exit 1
}

# Update the existing syfer installation
updateSyfer() {
	cd $HOME/.local/src/syfer-core

	echo "Pulling new commits..."
	git pull
	buildSyfer
	exit 0
}

# Open a wallet using syferwallet
openWallet() {
	which syferwallet > /dev/null 2>&1 || WALLETBIN="$BIN_DIR/syferwallet"

	[ "$TESTNET" = 1 ] && WALLET_ARGS="$WALLET_ARGS --testnet"
	[ ! -z "$DAEMON_ADDR" ] && WALLET_ARGS="$WALLET_ARGS --daemon-address=$DAEMON_ADDR"
	
	echo "$WALLETBIN --wallet-file $1 $WALLET_ARGS"
#	$WALLETBIN --wallet-file "$1" $WALLET_ARGS
	exit 0
}

# Add the build directory to $PATH?
addToPath() {
	printf ">>> Add the build directory to PATH? [y/N] "
	read addToPath

	case $addToPath in
		"y"|"Y") echo "export PATH=\"$PATH:$BIN_DIR\" >> $HOME/.bashrc";;
		"n"|"N"|*) echo "Don't forget to add '$BIN_DIR' to your PATH later";;
	esac
	return
}

# Show available arguments - ie. Help
scriptHelp() {
	cat << EOF
Syfer Core Deployment Script
==============================

General Flags
  --help    | -h       Show this help text
  --dev     | -d       Use the 'development' branch
  --master  | -m       Use the 'master' branch (default)
  --install | -i       Install syfer-core
  --update  | -u       Update syfer-core

Wallet Control
  --open    | -o       Open a wallet (--open <file>)
  --testnet | -t       Use testnet
  --daemon  | -D       Specify a custom daemon URL
EOF
	exit 0
}

## The actual script ##
case $1 in
	"") scriptHelp;;
esac

while test "$#" -gt 0; do
	case "$1" in
		"--master"|"-m") echo "Using master branch" && BRANCH="master";;
		"--dev"|"-d") echo "Using development branch" && BRANCH="development";;
	
		"--install"|"-i") INSTALL=1 UPDATE=0;;
		"--update"|"-u") INSTALL=0 UPDATE=1;;

		"--open"|"-o") shift && WALLET="$1";;
		"--testnet"|"-t") TESTNET=1;;
		"--daemon"|"-D") shift && DAEMON_ADDR="$1";;

		"--help"|"-h") scriptHelp;;
	esac
	shift
done
[ "$INSTALL" = "1" ] && installDeps && getSyfer
[ "$UPDATE" = "1" ] && updateSyfer
[ ! -z "$WALLET" ] && openWallet "$WALLET"
