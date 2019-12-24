//-------------------------------------------------------------
//-------------------------------------------------------------
//------- STOP. This file is created via C++. -----------------
//------- If you are modifying it then you should -------------
//------- probably be modifying the C++ code that -------------
//------- creates it instead ----------------------------------
//-------------------------------------------------------------
//-------------------------------------------------------------


//-------------------------------------------------------------
// Documentation created: Tue, 24 Dec 2019 23:24:12 GMT
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
		this.typedefs = [];
		this.namespaces = [];
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
	/**
	 *	@param inType - "class", "struct", "enum", "function", "variable", "enumValue"
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


/**
 *	@param name - should not be coverted to lower case
 *	@param description - description of class. Needs escaped characters
 *	@param partialTokens - array. Should all be lower case
 *	@param owner - can be left blank
 */
function addToContainers_class(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.classes.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.classes.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "class", owner));
	
	searchTrie.insert(name);
}

function addToContainers_struct(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.structs.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.structs.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "struct", owner));
	
	searchTrie.insert(name);
}

function addToContainers_enum(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.enums.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.enums.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "enum", owner));
	
	searchTrie.insert(name);
}

function addToContainers_function(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.functions.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.functions.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "function", owner));
	
	searchTrie.insert(name);
}

function addToContainers_variable(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.variables.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.variables.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "variable", owner));
	
	searchTrie.insert(name);
}

function addToContainers_enumValue(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.enumValues.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.enumValues.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "enumValue", owner));
	
	searchTrie.insert(name);
}

function addToContainers_typedef(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.typedefs.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.typedefs.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "typedef", owner));
	
	searchTrie.insert(name);
}

function addToContainers_namespace(name, description, relativePath, partialTokens, owner)
{
	var nameLowerCase = name.toLowerCase();

	if (searchTokenToPaths.has(nameLowerCase) == false)
	{
		searchTokenToPaths.set(nameLowerCase, new SearchPaths());
	}
	searchTokenToPaths.get(nameLowerCase).exactMatches.namespaces.push(relativePath);
	
	for (var i = partialTokens.length - 1; i >= 0; --i)
	{
		var tkn = partialTokens[i];
		if (searchTokenToPaths.has(nameLowerCase) == false)
		{
			searchTokenToPaths.set(nameLowerCase, new SearchPaths());
		}
		searchTokenToPaths.get(nameLowerCase).partialMatches.namespaces.push(relativePath);
	}
	
	pathToBasicInfo.set(relativePath, new BasicSearchInfo(name, description, "namespace", owner));
	
	searchTrie.insert(name);
}


//------------------------------------------------
// ------- Trie Implementation -------
//------------------------------------------------
//	Trie implementation for the search box suggestions

/**
 *	A single node in a trie
 */
class TrieNode
{
	constructor()
	{
		// The letter attached to this node. Only the root node should keep this empty
		this.letter = "";
		// Whether this node is the last letter of a word
		this.bIsWord = false;
		// if bIsWord == true then this equals the word
		this.word = "";
		// Child nodes
		this.children = [];
	}
};

TrieNode.prototype.recursiveFindChildThenAdd = function(str, index)
{
	// Check if at end of string
	if (index == str.length)
	{
		this.bIsWord = true;
		this.word = str;
		return;
	}

	for (var i = 0; i < this.children.length; ++i)
	{
		if (this.children[i].letter == str.charAt(index))
		{
			if (index >= str.length)
			{
				console.error("recursiveFindChildThenAdd(): index >= str.length");
			}

			index++;
			this.children[i].recursiveFindChildThenAdd(str, index);
			return;
		}
	}

	/* If here then none of the children had the letter. Create a new node with the letter */
	var newNode = new TrieNode();
	newNode.letter = str.charAt(index);
	this.children.push(newNode);

	index++;
	newNode.recursiveFindChildThenAdd(str, index);
};

/** Add a word to the trie. This is intended to be called on the root node */
TrieNode.prototype.insert = function(str)
{
	this.recursiveFindChildThenAdd(str, 0);
};

/**
 * Get words in the trie that prefix a string
 *
 * @param str - string to get words for
 * @param index - current position in str
 * @param words - array of words that str is a prefix for
 */
TrieNode.prototype.recursiveGetPrefixedWords = function(str, index, words)
{
	index++;
	/* Check if this node is a word and it is the same or greater length than str. If yes then add it */
	if (this.bIsWord == true && str.length <= index)
	{
		words.push(this.word);
	}

	if (str.length <= index)
	{
		for (var i = 0; i < this.children.length; ++i)
		{
			this.children[i].recursiveGetPrefixedWords(str, index, words);
		}
	}
	else
	{
		for (var i = 0; i < this.children.length; ++i)
		{
			if (this.children[i].letter.toLowerCase() == str.charAt(index).toLowerCase())
			{
				this.children[i].recursiveGetPrefixedWords(str, index, words);
				/* Have commented this return. Reason is the search is case insensitive so if user types "a"
				then the entry "apple" and "Apple" are both possibilities so cannot return since one might
				get skipped */
				//return;
			}
		}
	}
};


/** Class declaration: a trie. Contains a hashset and a root node */
class Trie
{
	constructor()
	{
		// Should I be using new here? Or is there a better way?

		// A container that holds all the words in the trie
		this.wordsSet = new Set();
		// The root node of the trie
		this.RootNode = new TrieNode();
	}
};


/**
 * Check whether a string has already been added to the trie
 *
 *	@return - true if the string has already been added to the trie.
 */
Trie.prototype.contains = function(str)
{
	return this.wordsSet.has(str);
};

/**
 * Try add a string to the trie
 *
 * @param str - the string to add to the trie
 */
Trie.prototype.insert = function(str)
{
	if (str.length == 0)
	{
		// Avoid adding empty string
		return;
	}

	if (this.contains(str))
	{
		// Avoid adding string if it's already in the trie
		return;
	}

	this.RootNode.insert(str);
	this.wordsSet.add(str);
};

/**
 * Return all the words in the trie that have str as a prefix
 *
 * @param str - prefix string
 * @return - array of words that have str as a prefix
 */
Trie.prototype.getPrefixedWords = function(str)
{
	var words = [];

	if (str.length == 0)
	{
		return words;
	}

	this.RootNode.recursiveGetPrefixedWords(str, -1, words);

	return words;
};

// ------------- End trie implementaion ---------------


// The trie for search suggestions for the search box on the documentation homepage
var searchTrie = new Trie();


addToContainers_class("ATestForDocTool", "Fill out your copyright notice in the Description page of Project Settings.", "Classes/ATestForDocTool/ATestForDocTool.html", ["test", "for", "doc", "tool"], "");
addToContainers_function("ATestForDocTool", "Sets default values for this actor\'s properties", "Classes/ATestForDocTool/Functions/ATestForDocTool.html", ["test", "for", "doc", "tool"], "ATestForDocTool");
addToContainers_function("BeginPlay", "Called when the game starts or when spawned", "Classes/ATestForDocTool/Functions/BeginPlay.html", ["begin", "play"], "ATestForDocTool");
addToContainers_function("Tick", "Called every frame", "Classes/ATestForDocTool/Functions/Tick.html", [], "ATestForDocTool");
addToContainers_variable("TestInt", "A test integer", "Classes/ATestForDocTool/Variables/TestInt.html", ["test", "int"], "ATestForDocTool");
