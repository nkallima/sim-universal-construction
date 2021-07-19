#!/bin/sh

echo 'Generating Doxygen code documentation...'
make clean docs

##### Configure git.
# Set the push default to simple i.e. push only the current branch.
git config --global push.default simple

# Only upload if Doxygen successfully created the documentation.
# Check this by verifying that the html directory and the file html/index.html
# both exist. This is a good indication that Doxygen did it's work.
if [ -d "build/docs/html" ] && [ -f "build/docs/html/index.html" ]; then
    echo 'Removing outdated documentation'
    rm -rf docs/*
    cp -r ./build/docs/html/* ./docs

    git checkout gh-pages
    echo 'Uploading documentation to the gh-pages branch...'
    git add ./docs/*

    make clean

    git commit -m "Documentation upload." 

    git push --force "https://github.com/nkallima/sim-universal-construction.git" > /dev/null 2>&1
else
    echo '' >&2
    echo 'Warning: No documentation (html) files have been found!' >&2
    echo 'Warning: Not going to push the documentation to GitHub!' >&2
    exit 1
fi
