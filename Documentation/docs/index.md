# Welcome to Mbrpistoni personal Unreal documentation

Here I will be adding explanations for how some systems work in Unreal (mostly AI related) and some "gotchas" and fixes for bugs I find along the way. You might end up here through UDN posts.

## Project layout

    mkdocs.yml                      # The configuration file in case you want to download this documentation and build a local site with MKDocs (https://www.mkdocs.org/)
    docs/
        index.md                    # The documentation homepage (MKDir file needed)
        Readme.mkdocs               # This very same document
        FromNavVolumesToNavData.md  # Explanation for what happens in the engine from the moment a Nav volume is streamed in within a sub-level to the moment when it's streamed out
