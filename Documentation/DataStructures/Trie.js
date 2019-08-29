//------------------------------------------------
 // ------- Trie Implementation -------
 //------------------------------------------------

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

    // Check if at end of string
    if (index + 1 == str.length)
    {
       newNode.bIsWord = true;
       newNode.word = str;
    }
    else	
    {
       index++;
       newNode.recursiveFindChildThenAdd(str, index);
    }
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
