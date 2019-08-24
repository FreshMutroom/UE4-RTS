

/** 
 *	Class declaration 
 */
TrieNode = function()
{
	var letter = '\0';
    var bIsWord = false;
    var children = [];
};

/** 
 *	@return - true if the string has already been added to the trie.
 */
TrieNode.prototype.contains = function(str)
{
	// I haven't declared this variable anywhere yet
	return filenamesSet.has(str);
};

TrieNode.prototype.recursiveFindChildThenAdd(str, index)
{
	for (var i = 0; i < this.children.length; ++i)
	{
		if (this.children[i].letter == str[index])
		{
			// assert(index < str.length);
			index++;
			recursiveFindChildThenAdd(str, index);
			return;
		}
	}
	
	/* If here then none of the children had the letter. Create a new node with the letter */
	var newNode = new TrieNode();
	newNode.letter = str[index];
	this.children.push(newNode);
	
	// Check if at end of string
	if (index + 1 == str.length)
	{
		newNode.bIsWord = true;
	}
	else	
	{
		index++;
		newNode.recursiveFindChildThenAdd(str, index);
	}
};

/* Add a word to the trie. This is intended to be called on the root node */
TrieNode.prototype.insert = function(str) 
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
	
	recursiveFindChildThenAdd(str, 0);
};

/* Hoping here that words is pass by reference */
TrieNode.prototype.recursiveGetPrefixedWords = function(str, index, words[])
{
	/* Check if this node is a word. If yes then add it */
	if (this.bIsWord == true)
	{
		words.push(str.substring(0, index + 1));
	}
	
	index++;
	for (var i = 0; i < this.children.length; ++i)
	{
		if (this.children[i].letter == str[index])
		{
			this.children[i].recursiveGetPrefixedWords(str, index, words);
			return;
		}
	}
};

/* This is intended to be called on the root node 
@param str - the prefix string */
TrieNode.prototype.getPrefixedWords = function(str)
{	
	var words = [];
	
	if (str.length == 0)
	{
		return words;
	}
	
	recursiveGetPrefixedWords(str, 0, words); 
	
	return words;
};
