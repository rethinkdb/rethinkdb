# Contributing

We're happy you want to contribute! You can help us in different ways:

- [Open an issue][1] with suggestions for improvements
- Fork this repository and submit a pull request
- Improve the [documentation][2] (separate repository)

[1]: https://github.com/rethinkdb/rethinkdb/issues
[2]: https://github.com/rethinkdb/docs

To submit a pull request, fork the [RethinkDB repository][3] and then clone your fork:

    git clone git@github.com:<your-name>/rethinkdb.git

[3]: https://github.com/rethinkdb/rethinkdb

Make your suggested changes, `git push` and then [submit a pull
request][4]. Note that before we can accept your pull requests, you
need to accept our [Developer Certificate of Origin][5] and sign your commits.

[4]: https://github.com/rethinkdb/rethinkdb/compare/
[5]: https://github.com/rethinkdb/rethinkdb/blob/main/CONTRIBUTING.md#developers-certificate-of-origin

## Building the admin UI

The code for the admin UI is now in a separate branch,
[`old_admin`][old_admin].  It is used to generate the file
`src/gen/web_assets.cc`, which contains the static content served by
RethinkDB's admin UI.  Development instructions are in that repo.

[old_admin]: https://github.com/rethinkdb/rethinkdb/tree/old_admin

## Resources

Some useful resources to get started:
* [Building RethinkDB][6] from source
* Overview of [what to find where][7] in the server source directory
* Introduction to the [RethinkDB driver protocol][8]
* [C++ coding style][9] for the RethinkDB server

[6]: http://rethinkdb.com/docs/build/
[7]: src/README.md
[8]: http://rethinkdb.com/docs/driver-spec/
[9]: STYLE.md

## Developer's Certificate of Origin

Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I have the right to submit it under the open source
license indicated in the file; or

(b) The contribution is based upon previous work that, to the best of my knowledge, is covered under an appropriate open
source license and I have the right under that license to submit that work with modifications, whether created in whole
or in part by me, under the same open source license (unless I am permitted to submit under a different license), as
indicated in the file; or

(c) The contribution was provided directly to me by some other person who certified (a), (b) or (c) and I have not
modified it.

(d) I understand and agree that this project and the contribution are public and that a record of the contribution (
including all personal information I submit with it, including my sign-off) is maintained indefinitely and may be
redistributed consistent with this project or the open source license(s) involved.
