Substance (4.11.0.14)
===================

Substance Engine, integrated with UE4, lets users directly import Substance (.sbsar) files in their UE4 project. 
When loaded, the plugin will expose (dynamically create widgets) the substance's parameters, so that users can tweak 
them in the Unreal Editor and ask Substance Engine to update the textures defined in the same Substance file. 

These parameters can also be accessed via scripting / BluePrint for runtime recomputation of the texture maps.
Substance textures are shader-agnostic and will work as input to any shader.

Overall, the main advantages of using Substance files instead of traditional bitmap textures are:

+ Productivity via in-engine look development and texture aspect tweaking.
+ Reduction of texture file size on disk: faster downloads, happier players who don't need to wait days before playing this new shiny game.
+ Dynamic texturing via runtime modifications / tweaking of Substance textures.

Support
-------

You can discuss about the integration and ask questions on our forum: [http://forum.allegorithmic.com](http://forum.allegorithmic.com).

You can report bugs at [bugs+ue4@allegorithmic.com](mailto:bugs+ue4@allegorithmic.com), please add "UE4" in the title of the email, and as much information as possible (version number, etc.).

Getting Up And Running
----------------------

You will follow the same steps as outlined by Epic on the UnrealEngine Repository. However there is one key change:

+ Instead of downloading the Unreal Engine source from Epic's repository, you use the Allegorithmic Repository.
