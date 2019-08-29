//-------------------------------------------------------------
//-------------------------------------------------------------
//------- STOP. This file is created via C++. -----------------
//------- If you are modifying it then you should -------------
//------- probably be modifying the C++ code that -------------
//------- creates it instead ----------------------------------
//-------------------------------------------------------------
//-------------------------------------------------------------


class ArraysOfFolderPaths
{
	constructor()
	{
		this.classes = [];
		this.structs = [];
		this.variables = [];
		this.functions = [];
		this.enums = [];
		this.enumValues = [];
	}
};

class SearchPaths
{
	constructor(inFolderPath)
	{
		this.partialMatches = new ArraysOfFolderPaths();

		this.exactMatches = new ArraysOfFolderPaths();
	}
};

class BasicSearchInfo
{
	/**	 *	@param inType - "class", "struct", "enum", "function", "variable", "enumValue"
	 *	@param inOwner - can be left blank. This is the "item" that owns this item
	 *	e.g. something of type "function" will usually have an owner that is the name of the
	 *	class/struct that it was declared in
	 */
	constructor(inName, inDescription, inType, inOwner)
	{
		// name supports case
		this.name = inName;
		this.description = inDescription;
		this.type = inType;
		this.owner = inOwner;
	}
};


// Key = a string, value = class SearchPaths
var searchTokenToPaths = new Map();

// Maps folder path to basic info about the item such as it's description
var pathToBasicInfo = new Map();


if (searchTokenToPaths.has("udoctooltestuobject") == false)
{
	searchTokenToPaths.set("udoctooltestuobject", new SearchPaths());
}
searchTokenToPaths.get("udoctooltestuobject").exactMatches.classes.push("Docs/Classes/UDocToolTestUObject.html");
if (searchTokenToPaths.has("doc") == false)
{
	searchTokenToPaths.set("doc", new SearchPaths());
}
searchTokenToPaths.get("doc").partialMatches.classes.push("Docs/Classes/UDocToolTestUObject.html");
if (searchTokenToPaths.has("tool") == false)
{
	searchTokenToPaths.set("tool", new SearchPaths());
}
searchTokenToPaths.get("tool").partialMatches.classes.push("Docs/Classes/UDocToolTestUObject.html");
if (searchTokenToPaths.has("test") == false)
{
	searchTokenToPaths.set("test", new SearchPaths());
}
searchTokenToPaths.get("test").partialMatches.classes.push("Docs/Classes/UDocToolTestUObject.html");
if (searchTokenToPaths.has("u") == false)
{
	searchTokenToPaths.set("u", new SearchPaths());
}
searchTokenToPaths.get("u").partialMatches.classes.push("Docs/Classes/UDocToolTestUObject.html");
if (searchTokenToPaths.has("object") == false)
{
	searchTokenToPaths.set("object", new SearchPaths());
}
searchTokenToPaths.get("object").partialMatches.classes.push("Docs/Classes/UDocToolTestUObject.html");
pathToBasicInfo.set("Docs/Classes/UDocToolTestUObject.html", new BasicSearchInfo("UDocToolTestUObject", "A test comment blah blah blah", "class", ""));

if (searchTokenToPaths.has("fsometeststruct") == false)
{
	searchTokenToPaths.set("fsometeststruct", new SearchPaths());
}
searchTokenToPaths.get("fsometeststruct").exactMatches.structs.push("Docs/Structs/FSomeTestStruct.html");
if (searchTokenToPaths.has("some") == false)
{
	searchTokenToPaths.set("some", new SearchPaths());
}
searchTokenToPaths.get("some").partialMatches.structs.push("Docs/Structs/FSomeTestStruct.html");
if (searchTokenToPaths.has("test") == false)
{
	searchTokenToPaths.set("test", new SearchPaths());
}
searchTokenToPaths.get("test").partialMatches.structs.push("Docs/Structs/FSomeTestStruct.html");
if (searchTokenToPaths.has("struct") == false)
{
	searchTokenToPaths.set("struct", new SearchPaths());
}
searchTokenToPaths.get("struct").partialMatches.structs.push("Docs/Structs/FSomeTestStruct.html");
pathToBasicInfo.set("Docs/Structs/FSomeTestStruct.html", new BasicSearchInfo("FSomeTestStruct", "A test struct for documentation", "struct", ""));

