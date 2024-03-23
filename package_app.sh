#!bin/sh

echo Package NAP app.
echo Usage: package_app [target] [build directory] [code signature]

# Check if target is specified
if [ "$#" -lt "1" ]; then
  echo "Specify a target."
  exit 0
fi

# Install jq utility to parse json
if [ "$(uname)" == "Darwin" ]; then
  brew install jq
elif [ "$(uname)" == "Linux" ]; then
  echo install jq linux
else
  # Windows
  curl -L -o jq.exe https://github.com/stedolan/jq/releases/latest/download/jq-win64.exe
fi

target=$1
if [ $# = "1" ]; then
  build_directory="build"
else
  build_directory=$2
fi

# Remove bin directory from previous builds
# This is important otherwise artifacts from previous builds could be included in the app installation
echo Cleaning previous build output...
rm -rf $build_directory/bin

# Generate the build directory
cmake -S . -B $build_directory

# Build the specified target
cmake --build $build_directory --target $target --config Release --parallel 8

# Run cmake install process
cmake --install build --prefix install

# Read app Title from project json
if [ "$target" == "napkin" ]; then
  if [ "$(uname)" == "Darwin" ]; then
    # Add app bundle file extension on MacOS
    app_title=Napkin.app
  else
    app_title=Napkin
  fi
else
  if [ "$(uname)" == "Darwin" ]; then
    # Add app bundle file extension on MacOS
    app_title=`jq -r '.Title' build/bin/$target.json`.app
  elif [ "$(uname)" == "Linux" ]; then
    app_title=`jq -r '.Title' build/bin/$target.json`
  else
    app_title=`./jq -r '.Title' build/bin/$target.json`
  fi
fi

# Cleaning previous install, if any
echo Cleaning previous install output...
rm -rf install/$app_title

# Rename output directory to app title
mv install/MyApp install/$app_title

# Codesign MacOS app bundle
if [ "$(uname)" == "Darwin" ]; then
  echo Codesigning MacOS bundle...
  if [ "$#" -lt "3" ]; then
    codesign --deep -s - -f $install_location
  else
    codesign --deep -s "$3" -f -i "com.$target.napframework.www" -f $install_location
  fi
fi

# Remove the build directory if it wasn't specified
if [ $# = "1" ]; then
  echo Removing build directory...
  rm -rf build
fi

# Remove local installation of jq.exe
if [ "$(uname)" != "Darwin" ] && [ "$(uname)" != "Linux" ]; then
  echo Removing local installation of jq.exe...
  rm jq.exe
fi
