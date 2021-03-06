# papercut
Creating papercut art in svg format
![Dog SVG](https://cdn.rawgit.com/jmlien/papercut/246835f9/chinese/zodiac/dog.svg)

* This project contains 
  1. code that converts a dual-color bitmap image (png/jpg) to svg format 
  2. a collection of papercut arts created by the program
  3. a collection of free svg papercut arts collected from internet

## Regarding the code

### Compile the code
* Usg cmake to build the code, the target 2svg will be created.
* visual studio 2012 solution file is also provided under "vc_files"

### Use the code
* **Usage**: 2svg -ct color_intensity_threshold -st contour_size_threshold -smooth amount -simplify amount -invert image
   * -ct color_intensity_threshold : This value is used to separate black from white.
   * -st contour_size_threshold : Ignore polygon with less than the "contour_size_threshold" vertices
   * -smooth amount : Smooth the polygon boundary, larger amount means more smooth
   * -simplify amount : Simplify the polygons using Douglas Peucker algorithm. Smaller amount means more points are kept. 
   * -invert : Invert color in the image
* Examples of how the code is used are provide in "examples" folder
  * See *.bat files in the folder
* For instance, to convert totoro.jpg to svg: 2svg -ct 80 -st 5 -smooth 0.75 .\totoro.jpg
* The input and output images are shown below

![Totoro](/examples/totoro.jpg) ![Totoro SVG](https://cdn.rawgit.com/jmlien/papercut/175b1e58/tosvg/examples/totoro.svg)
