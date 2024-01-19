# Learning Vulkan


## Hacky (but useful) Commands

### Clang Tidy and Format
```bash
find ./src -iname *.h -o -iname *.cpp | xargs clang-tidy --format-style=file --fix --fix-errors && find ./src -iname *.h -o -iname *.cpp | xargs clang-format -i --style=microsoft
```

#### Just Clang Format
```bash
find ./src -iname *.h -o -iname *.cpp | xargs clang-format -i --style=microsoft
```

#### Just Clang Tidy
```bash
find ./src -iname *.h -o -iname *.cpp | xargs clang-tidy --format-style=file --fix --fix-errors
```