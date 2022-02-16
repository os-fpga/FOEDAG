.. _tutorial_git_flow:

Git flow
------------------

On this page, you can find rules and tips on how to use git with this project.

Fork
====

First of all, to contribute, you need to create a fork of the project on GitHub.
To keep your fork always in sync:
1. Specify a remote upstream repo to sync with your fork

.. code-block:: git

 git remote add upstream https://github.com/os-fpga/FOEDAG.git


2. Verify using

.. code-block:: git

 git remote -v


3. Fetch branches and commits from the upstream repo

.. code-block:: git

 git fetch upstream

4. Checkout your fork’s local branch that you want to sync and merge chenges from upstream branch. For example 'main'

.. code-block:: git

 git checkout main
 git merge upstream/main

5. Push changes to update your fork

.. code-block:: git

 git push origin main

Branch
======

:shared branch:
  a branch that several developers are working on at once

:private branch:
  a branch that only one developer is working on

Please, don't use git merge to sync your private branch. This makes the history tangled.
With a regular rebase you can update your feature branch with the default branch (or any other branch). This is an important step for Git-based development strategies.

.. code-block:: git

 git checkout feature-branch
 git fetch origin main:main
 git rebase main
 git push --force origin feature-branch


.. note:: git rebase rewrites the commit history. It can be harmful to do it in shared branches. It can cause complex and hard to resolve merge conflicts.

Pull request
============
To merge your branch with the main fork, create a pull request by GitHub
