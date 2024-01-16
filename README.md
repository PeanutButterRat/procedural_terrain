<a name="readme-top"></a>
<div align="center">
  <a href="https://github.com/PeanutButterRat/procedural_terrain">
      <img src="icons/ProceduralTerrain.svg" alt="Logo" height="80">
    </a>
  <br />
  <h3 align="center">ProceduralTerrain</h3>
  <p align="center">
    A quick and easy 3D procedural generation module for the Godot Game Engine.
    <br />
    <br />
    <a href="https://github.com/PeanutButterRat/procedural_terrain/issues">Report Bug</a>
    ·
    <a href="https://github.com/PeanutButterRat/procedural_terrain/issues">Request Feature</a>
  </p>
</div>


<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#compiling-the-engine">Compiling the Engine</a></li>
        <li><a href="#compiling-the-templates">Compiling the Templates</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

![Terrain Screen Shot](/assets/terrain-close.jpg)

ProceduralTerrain is a simple 3D procedural generation module for Godot that offers solid functionality. It offers a wide variety of options to generate your own terrain for your games 
made with the Godot Engine. While it's feature set is relatively small, it offers enough to fufill many demands when it comes to procedural generation.

Please feel free to use or modify the project as desired.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

To integrate the module into Godot Engine, you'll have to download and compile the engine from source. To do this follow the steps shown below. 

### Prerequisites

[Follow the steps in the Godot Documentation](https://docs.godotengine.org/en/stable/contributing/development/compiling/index.html) to get your setup ready to compile the engine from source.

### Compiling the Engine

1. Clone the repository or download the source zip file from GitHub
   ```sh
   git clone https://github.com/PeanutButterRat/procedural_terrain
   ```
2. Move the repository to godot/modules folder or create a separate folder to hold your custom modules and move it there. I prefer the custom modules workflow because it keeps everything separated, so I will be demonstrating that here.
   ```sh
   mkdir your_custom_modules_folder
   mv procedural_terrain your_custom_modules_folder
   ```
3. You are now ready to compile! Navigate your Godot source folder and run the ```scons``` command. If you simply dropped procedural_terrain in the godot/modules and don't have a separate folder for any other modules, you can omit the ```custom_modules``` flag.
   ```sh
   cd path_to_godot_folder
   scons platform=your_platform target=editor custom_modules=path_to_your_custom_modules_folder
   ```
3. You can find the compiled editor in the ```godot/bin``` folder.

### Compiling the Templates
In order to export your game you also have to compile the export templates which are just compliations of the engine without the editor attached to it. To do this you can follow the same steps above and just change the ```target``` flag when compiling to ```template_debug``` for the debug template or ```template_release``` for the release template.
```sh
cd path_to_godot_folder
scons platform=your_platform target=template_debug custom_modules=path_to_your_custom_modules_folder
scons platform=your_platform target=template_release custom_modules=path_to_your_custom_modules_folder
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

To generate terrain, just add a ProceduralTerrain node to your scene and play around with the parameters.

![Generation Example](/assets/terrain-far.jpg)


<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- CONTRIBUTING -->
## Contributing

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with and present your idea there. Any input or contributions is greatly appreciated!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/YourCoolFeature`)
3. Commit your Changes (`git commit -m 'Add some YourCoolFeature'`)
4. Push to the Branch (`git push origin feature/YourCoolFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Eric Brown - [GitHub](https://github.com/PeanutButterRat) - ebrown5676@gmail.com

Project Link: [https://github.com/PeanutButterRat/procedural_terrain](https://github.com/PeanutButterRat/procedural_terrain)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

Here are some awesome resources that were crucial to the development to this application.

* [Sebastian Lague's Procedural Generation Series](https://www.youtube.com/watch?v=wbpMiKiSKm8&list=PLFt_AvWsXl0eBW2EiBtl_sxmDtSgZBxB3)
* [README Template](https://github.com/othneildrew/Best-README-Template)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

