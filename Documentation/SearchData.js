
class ArrayOfFolderPaths
{
	constructor()
	{
		this.array = [];
	}
};


class ItemInfo
{
	constructor(inType, inFolderPath, inComment)
	{
		// class, function, enum value, etc
		this.type = "";
		this.folderPath = "";
		this.comment = inComment;
	}
};


// Maps a name of something to array of paths to files for it e.g."Health" maps to an 
// array for AInfantry::Health, ABuilding::Health, etc. 
var nameToInfo = new Map();

if (nameToInfo.has("ABuilding") == false)
{
	nameToInfo.set("ABuilding", ArrayOfFolderPaths());
	
}
nameToInfo["ABuilding"].push(ItemInfo("class", "Docs/ABuilding", "blah blah");
