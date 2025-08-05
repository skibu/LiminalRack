# dep directory README

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
