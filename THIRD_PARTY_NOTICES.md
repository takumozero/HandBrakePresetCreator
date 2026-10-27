# Third-party notices

## HandBrake preset data

The built-in preset entries in `presets.json` originate from the HandBrake project.
Copyright belongs to the HandBrake project and its contributors. This repository
does not claim authorship of their preset names, descriptions, or parameter data.

- Upstream: https://github.com/HandBrake/HandBrake
- Preset sources: https://github.com/HandBrake/HandBrake/tree/master/preset
- License text supplied with HandBrake: [docs/licenses/HandBrake-COPYING.txt](docs/licenses/HandBrake-COPYING.txt)

The provided export was prepared as a public template on 2026-10-27: personal
custom preset content was excluded, while default-selection and expanded-folder
state were reset. Encoding parameters and preset-version metadata were retained.
Preserve this notice and the upstream license text when redistributing the data.

## GUI and parameter research

The application's Qt forms, JSON model, theme system, and test code are original
implementations. HandBrake's publicly available GUI layout and parameter enums
were consulted as interface/reference data; HandBrake implementation files and
binaries are not incorporated into the application source.

Reference links are recorded in `docs/filter-reference.md` and `handbrakeoptions.h`.

## Qt

Qt is an external build/runtime dependency. It is not vendored in this repository.
Qt and redistributed plugins/libraries retain their respective copyright and
license terms. Include the license/notice materials applicable to the Qt build
you distribute when preparing a binary package.

- https://www.qt.io/licensing/
- https://doc.qt.io/qt-6/windows-deployment.html

HandBrake and Qt names identify their respective projects. This editor is not
affiliated with or endorsed by those projects.
