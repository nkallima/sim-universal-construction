#!/bin/sh

echo 'Setting up the script...'
set -e


# Get the current branch
git clone -b v2.5.0-dev https://github.com/nkallima/sim-universal-construction.git

echo 'Generating Doxygen code documentation...'
cd sim-universal-construction
make clean docs
cd ..

##### Configure git.
# Set the push default to simple i.e. push only the current branch.
git config --global push.default simple

# Pretend to be an user called Travis CI.
git config user.name "Travis CI"
git config user.email "travis@travis-ci.org"

# Need to create a .nojekyll file to allow filenames starting with an underscore
# to be seen on the gh-pages site. Therefore creating an empty .nojekyll file.
# Presumably this is only needed when the SHORT_NAMES option in Doxygen is set
# to NO, which it is by default. So creating the file just in case.
echo "" > .nojekyll

# Only upload if Doxygen successfully created the documentation.
# Check this by verifying that the html directory and the file html/index.html
# both exist. This is a good indication that Doxygen did it's work.
if [ -d "sim-universal-construction/build/docs/html" ] && [ -f "sim-universal-construction/build/docs/html/index.html" ]; then
    echo 'Removing outdated documentation'
    rm -rf docs/*
    cp -r sim-universal-construction/build/docs/html/* ./docs
    rm -rf sim-universal-construction

    echo 'Uploading documentation to the gh-pages branch...'
    git add --all

    git commit -m "Deploy code docs to GitHub Pages Travis build: ${TRAVIS_BUILD_NUMBER}" -m "Commit: ${TRAVIS_COMMIT}"

    git push --force "https://github.com/nkallima/sim-universal-construction.git" > /dev/null 2>&1
else
    echo '' >&2
    echo 'Warning: No documentation (html) files have been found!' >&2
    echo 'Warning: Not going to push the documentation to GitHub!' >&2
    exit 1
fi
