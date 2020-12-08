# Contribution

If you wish to contribute to the project:
- Install `clang-format` package (version >= 10)
- After cloning the repository you need to run `git config --local core.hooksPath .git_hooks` to enable recommended code style checkes (placed in .clang_format file) during git commit. If the tool discovers incosistencies, it will create a patch file. Please follow the instructions to apply the patch before opening a pull request.
