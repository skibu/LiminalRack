# dep/ directory README

Though this directory is called dep for dependencies it should be noted that
it actually consists of git "submodules". These submodules are separate projects
that are basically linked in to this project.

Info on submodules at 
https://www.reddit.com/r/C_Programming/comments/1biri2s/how_to_push_a_git_repo_dependency_to_my_git_repo/
* Article Tutorial: https://github.blog/2016-02-01-working-with-submodules/
* Video Tutorial: https://youtu.be/wTGIDDg0tK8

The configuration of the submodules is contained in the .gitmodules file. But
you should not edit this file manually. Instead, you should use commands like
`git submodule update path/to/local/copy` to update things.

If you need to make modifications to any of the submodules you should fork the
project into your own github site, make the modifications to your forked version,
and then use `git submodule update path` and check in the changes for your changes to 
take effect.

## Example of making change
If need to make a change in one of the submodules one can fork the submodule and then point 
Liminal Rack to repo to the fork. For example:

```
# Make the change
git submodule set-url -- dep/oui-blendish https://github.com/skibu/oui-blendish.git
# Commit the change
git add .gitmodules
git commit
git push
```

At this point you should be able to edit a file using VSCode in the forked submodule.
Could not figure out how to check in modified file using VSCode, but could do it from command line. Using dep/oui-blendish/blendish.c as an example.
```
cd dep/oui-blendish/
# Confirm that file in submodule was edited
git status
# Stage file so can commit it
git add blendish.c
# Commit it
git commit -m "learning about submodules"
# Try pushing, though this might fail due to detached HEAD
git push
# If push failed due to detached head then do:
git push origin HEAD:master
```
