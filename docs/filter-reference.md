# Easy-view filters

The menus use HandBrake's active preset tables. Existing legacy or custom values
remain intact when loading a file. Changing a filter algorithm updates the
available presets and tunes for that algorithm. Select Custom to reveal its
expression field; changing away from Custom retains the inactive expression.

| Filter | Presets / modes | Purpose and tunes |
| --- | --- | --- |
| Detelecine | Off, Default, Custom | Removes telecine's repeated fields to recover film cadence. |
| Interlace detection | Off, Default, Less sensitive, Fast, Custom | Detects combing for selective deinterlacing. It does not reconstruct frames itself. |
| Decomb | Default, Bob, EEDI2, EEDI2 Bob, Custom | Selective deinterlacing; Bob emits separate frames for fields, while EEDI2 uses edge-directed interpolation. |
| Yadif | Default, Skip spatial check, Bob, Custom | Deinterlacing; skipping the spatial check trades work for a different detection/reconstruction behavior. |
| Bwdif | Default, Bob, Custom | Progressive reconstruction with a Bob mode for field-rate output. |
| HQDN3D | Ultralight, Light, Medium, Strong, Custom | Spatial and temporal noise reduction. |
| NLMeans | Ultralight, Light, Medium, Strong, Custom | Patch-based denoising. Tunes: None, Film, Grain, High motion, Animation, Tape, Sprite. |
| BM3D | Default, Medium, Strong, Custom | Block-matching denoising; requires a HandBrake version that provides this filter. |
| Chroma smooth | Off, Ultralight through Very strong, Custom | Smooths color noise. Tunes: None, Tiny, Small, Medium, Wide, Very wide. |
| LapSharp | Ultralight through Very strong, Custom | Laplacian sharpening. Tunes: None, Film, Grain, Animation, Sprite. |
| Unsharp | Ultralight through Very strong, Custom | Unsharp-mask sharpening. Tunes: None, Ultrafine, Fine, Medium, Coarse, Very coarse. |
| Deblock | Off, Ultralight through Very strong, Custom | Reduces compression blocks. Tune selects Small (4x4), Medium (8x8), or Large (16x16). |
| Colorspace | Off, BT.2020, BT.709, BT.601 SMPTE-C, BT.601 EBU, Custom | Converts color properties using HandBrake's preset transforms and tone-mapping parameters. |
| Grayscale | On / Off | Removes color. |

The full strength sequence, where supported, is Ultralight, Light, Medium,
Strong, Stronger, Very strong. Higher strength may remove detail or introduce
artifacts. Custom expressions are passed through to HandBrake for interpretation;
the editor does not run the filters or validate their full expression syntax.

## Sources

- [HandBrake filter preset and tune definitions](https://github.com/HandBrake/HandBrake/blob/master/libhb/param.c)
- [HandBrake filter GUI](https://github.com/HandBrake/HandBrake/blob/master/win/CS/HandBrakeWPF/Views/FiltersView.xaml)
- [HandBrake preset-to-GUI mapping](https://github.com/HandBrake/HandBrake/blob/master/win/CS/HandBrakeWPF/Services/Presets/Factories/JsonPresetFactory.cs)

The parameter names and enumerated values are reference data. The Qt controls,
dependency handling, theme system, and tests are implemented independently here.
