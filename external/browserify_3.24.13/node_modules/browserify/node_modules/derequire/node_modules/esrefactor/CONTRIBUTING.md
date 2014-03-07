# Contribution Guide

This page describes how to contribute changes to esrefactor.

Please do **not** create a pull request without reading this guide first. Failure to do so may result in the **rejection** of the pull request.

## Testing Workflow

To prepare the environment for testing, Node.js is required:

    npm install

There are two types of tests: unit test and coverage test. To run the
tests:

    npm test

If either the unit or the coverage test files, it will complain.
Any kind of regression, as spotted by the above tests, is **not tolerated**.

### Unit Tests

    node test/run.js

TODO: How to write a new test.

### Code Coverage

To ensure a good code coverage, [Istanbul](https://github.com/gotwarlost/istanbul) is invoked while running the unit tests:

    npm run-script coverage

## Review and Merge

When your branch is ready, send the pull request.

While it is not always the case, often it is necessary to improve parts of your code in the branch. This is the actual review process.

Here is a check list for the review:

* It does not break the test suite
* The coding style follows the existing one
* There is a reasonable amount of comment
* There is no typo and other related mistakes

