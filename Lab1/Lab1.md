## Lab 1 - Studying the C Parser

| Ankush Phulia | Vaibhav Bhagee |
| :-----------: | :------------: |
|  2014CS50279  |  2014CS50297   |

#### Flex generated Lexer

* ​

  ##### Generated Tables

  * Flex can generate a full state transition table for FA - a 1D array of (symbol, next state), indexed by current state, or by default smaller, 1D arrays
  * The transition table is compressed using double displacement - 2D table is flattened to 1D by
    * Mapping all non-white-space entries into some location of the array, keeping the order of elements in each row intact
    * Displacing each row by some amount such that no two non-white-space entries in each row occupy the same position

  ```C
  YY_DECL // macro for yylex()
  {
  	yy_state_type yy_current_state; //current state of the DFA
  	char *yy_cp, *yy_bp;
  	int yy_act; // action number to be taken in the current state
      
    	// checks if initialization of file pointers and buffers has not already been done
  	if ( !(yy_init) ) 
  		{
  		(yy_init) = 1;

  		if ( ! (yy_start) )
  			(yy_start) = 1;	// first start state
        
  		// Set global input and output streams to standard input and output if not already set to files
  		if ( ! yyin )
  			yyin = stdin;
  		if ( ! yyout )
  			yyout = stdout;

  		// This checks if a buffer is available for storing characters, if not, creates a buffer

  		if ( ! YY_CURRENT_BUFFER ) {
  			yyensure_buffer_stack ();
  			YY_CURRENT_BUFFER_LVALUE =
  				yy_create_buffer(yyin,YY_BUF_SIZE ); // creates a buffer to use
  		}

  		// Loads the buffer state
  		yy_load_buffer_state( ); // loads the buffer state in the relevant global variables
  		}

  	{
  #line 39 "c.l"

  #line 1035 "c.lex.cpp"

  	while ( /*CONSTCOND*/1 )		// The main lexing loop
  		{
  		yy_cp = (yy_c_buf_p);    // stores the location before entering in the loop, in the buffer
  		
  		*yy_cp = (yy_hold_char); 
  		
      /* yy_bp points to the position in yy_ch_buf of the start of
  		 * the current run.
  		 */
  		yy_bp = yy_cp;

  		yy_current_state = (yy_start); // starts the DFA at the start state

  yy_match:
  		do
  			{
  			YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)] ;

        // Check if this state is accepting, if yes, then save the character pointer and state
        // The lexing won't stop here as we want maximal munch

  			if ( yy_accept[yy_current_state] )
  				{
  				(yy_last_accepting_state) = yy_current_state;
  				(yy_last_accepting_cpos) = yy_cp;
  				}


  			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
  				{
  				yy_current_state = (int) yy_def[yy_current_state];
  				if ( yy_current_state >= 385 )
  					yy_c = yy_meta[(unsigned int) yy_c];
  				}

        // Decide the next state based on current state and the character
        // Double displacement technique has been used to flatten 2D tables to 1D tables
        // The character pointer is incremented
          
  			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
  			++yy_cp;
  			}
  		while ( yy_base[yy_current_state] != 699 );

  yy_find_action:
  		yy_act = yy_accept[yy_current_state];
  		if ( yy_act == 0 )
  			{ /* have to back up */
  			yy_cp = (yy_last_accepting_cpos);
  			yy_current_state = (yy_last_accepting_state);
  			yy_act = yy_accept[yy_current_state];
  			}

  		YY_DO_BEFORE_ACTION;

  		if ( yy_act != YY_END_OF_BUFFER && yy_rule_can_match_eol[yy_act] )
  			{
  			yy_size_t yyl;
  			for ( yyl = 0; yyl < yyleng; ++yyl )
  				if ( yytext[yyl] == '\n' )
      yylineno++;
  ;
  			}

  do_action:	/* This label is used only to access EOF actions. */
  		switch ( yy_act ) { // switch over actions
  			case 0: // back-up
  			// undo the effects of YY_DO_BEFORE_ACTION
  			*yy_cp = (yy_hold_char);
  			yy_cp = (yy_last_accepting_cpos);
  			yy_current_state = (yy_last_accepting_state);
  			goto yy_find_action;

  case 1: 	   // to account for C-style comments
  YY_RULE_SETUP
  #line 40 "c.l" // refer to line number to flag error, in case of problem
  { comment(); }
  	YY_BREAK
  case 2:
  YY_RULE_SETUP
  #line 41 "c.l"
  { /* consume //-comment */ }
  	YY_BREAK
  // cases for many tokens ommitted
  // ...
  case 108: 	   // account for ECHO
  YY_RULE_SETUP
  #line 155 "c.l"
  ECHO;
  	YY_BREAK
  #line 1644 "c.lex.cpp"
  case YY_STATE_EOF(INITIAL): // End of File Reached
  	yyterminate();

  	case YY_END_OF_BUFFER:
  		{
  		/* Amount of text matched not including the EOB char. */
  		int yy_amount_of_matched_text = (int) (yy_cp - (yytext_ptr)) - 1;

  		/* Undo the effects of YY_DO_BEFORE_ACTION. */
  		*yy_cp = (yy_hold_char);
  		YY_RESTORE_YY_MORE_OFFSET

  		if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_NEW )
  			{
  			/* We're scanning a new file or input source.  It's
  			 * possible that this happened because the user
  			 * just pointed yyin at a new source and called
  			 * yylex().  If so, then we have to assure
  			 * consistency between YY_CURRENT_BUFFER and our
  			 * globals.  Here is the right place to do so, because
  			 * this is the first action (other than possibly a
  			 * back-up) that will match for the new input source.
  			 */
  			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
  			YY_CURRENT_BUFFER_LVALUE->yy_input_file = yyin;
  			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status = YY_BUFFER_NORMAL;
  			}

  		/* Note that here we test for yy_c_buf_p "<=" to the position
  		 * of the first EOB in the buffer, since yy_c_buf_p will
  		 * already have been incremented past the NUL character
  		 * (since all states make transitions on EOB to the
  		 * end-of-buffer state).  Contrast this with the test
  		 * in input().
  		 */
  		if ( (yy_c_buf_p) <= &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] )
  			{ /* This was really a NUL. */
  			yy_state_type yy_next_state;

  			(yy_c_buf_p) = (yytext_ptr) + yy_amount_of_matched_text;

  			yy_current_state = yy_get_previous_state(  );

  			/* Okay, we're now positioned to make the NUL
  			 * transition.  We couldn't have
  			 * yy_get_previous_state() go ahead and do it
  			 * for us because it doesn't know how to deal
  			 * with the possibility of jamming (and we don't
  			 * want to build jamming into it because then it
  			 * will run more slowly).
  			 */
  			yy_next_state = yy_try_NUL_trans( yy_current_state );
  			yy_bp = (yytext_ptr) + YY_MORE_ADJ;

  			if ( yy_next_state )
  				{
  				/* Consume the NUL. */
  				yy_cp = ++(yy_c_buf_p);
  				yy_current_state = yy_next_state;
  				goto yy_match;
  				}
  			else
  				{
  				yy_cp = (yy_c_buf_p);
  				goto yy_find_action;
  				}
  			}

  		else switch ( yy_get_next_buffer(  ) )
  			{
  			case EOB_ACT_END_OF_FILE:
  				{
  				(yy_did_buffer_switch_on_eof) = 0;

  				if ( yywrap( ) )
  					{
  					/* Note: because we've taken care in
  					 * yy_get_next_buffer() to have set up
  					 * yytext, we can now set up
  					 * yy_c_buf_p so that if some total
  					 * hoser (like flex itself) wants to
  					 * call the scanner after we return the
  					 * YY_NULL, it'll still work - another
  					 * YY_NULL will get returned.
  					 */
  					(yy_c_buf_p) = (yytext_ptr) + YY_MORE_ADJ;

  					yy_act = YY_STATE_EOF(YY_START);
  					goto do_action;
  					}
  				else
  					{
  					if ( ! (yy_did_buffer_switch_on_eof) )
  						YY_NEW_FILE;
  					}
  				break;
  				}

  			case EOB_ACT_CONTINUE_SCAN:
  				(yy_c_buf_p) =
  					(yytext_ptr) + yy_amount_of_matched_text;

  				yy_current_state = yy_get_previous_state(  );

  				yy_cp = (yy_c_buf_p);
  				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
  				goto yy_match;

  			case EOB_ACT_LAST_MATCH:
  				(yy_c_buf_p) =
  				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)];

  				yy_current_state = yy_get_previous_state(  );

  				yy_cp = (yy_c_buf_p);
  				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
  				goto yy_find_action;
  			}
  		break;
  		}

  	default:
  		YY_FATAL_ERROR(
  			"fatal flex scanner internal error--no action found" );
  	} /* end of action switch */
  		} /* end of scanning one token */
  	} /* end of user's declarations */
  }
  ```

  ​



***



#### Bison generated Parser

* The generated parser is supposed to be LALR(1)

* ....

  ##### Generated Tables

  * Bison does not generate a LR parse table(state transition table for FA), but instead creates smaller, 1D arrays

  * As described for Flex, Bison uses double displacement to flatten the array

    * Set of displacements is `D[i]`, in the displacements table `D` for each row `i`. Position `(i, j)` in the table is mapped to position `D[i] + j` in the array

    * Thus in the final array made from the transition table, we index by state number and symbol number

    * If the next look-ahead symbol has number k, table entry for state n is 

      `T[ D[n] + k ]` for the final 1D array `T`

  * Some of the tables used : 
    * `yytranslate` - maps lexical token numbers to their symbol numbers
    * `yydefact` - 'default action', default reductions for each state
    * `yydefgoto` - 'default go to', maps (state, non-terminal) to state
    * `yyr1` - specifies the symbol number of the LHS of each rule - for next state during reduce moves
    * `yyr2` - specifies the number of symbols on the RHS of each rule - for popping stacks during reduce moves
    * `yytable` -  compressed representation of the actions to take in each state - negative for reductions, NINF for error detection
    * `yypgoto` - 'present go to', maps (previous state, non-terminal) to next state
    * `yypact` - 'present action', describes what to do in a given state. It is a directory into `yytable` indexed by state number
    * `yycheck` - used for various checks - legal bounds within portions of `yytable`

  ##### The `yyparse()` function

  ```C
  int yyparse (void)
  {
      int yystate; // current state
    
      int yyerrstatus; //No. of tokens to shift before error messages enabled

    	// state stack - stack of state numbers
      yytype_int16 yyssa[YYINITDEPTH]; // initialise, YYINITDEPTH = 200
      yytype_int16 *yyss; 			// bottom-of-stack pointer
      yytype_int16 *yyssp;			// top-of-stack pointer

      // semantic value stack - grows along with state stack
      YYSTYPE yyvsa[YYINITDEPTH]; 	 // initialise
      YYSTYPE *yyvs;				    // bottom-of-stack pointer
      YYSTYPE *yyvsp;				    // top-of-stack pointer

      YYSIZE_T yystacksize;

    int yyn;
    int yyresult;			// result of the parse which is returned to caller 
    int yytoken = 0;		// store the look-ahead token(as int)
    YYSTYPE yyval;		// semantic value of token to be pushed after red.

  /* Pop N symbols/states from semantic and state stacks respectively */
  #define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

    int yylen = 0;		// Length of RHS for reduce rules, 0 when no pop
    // intialise stacks
    yyssp = yyss = yyssa; 
    yyvsp = yyvs = yyvsa;
    yystacksize = YYINITDEPTH;

    YYDPRINTF ((stderr, "Starting parse\n"));

    yystate = 0;
    yyerrstatus = 0;
    yynerrs = 0;
    yychar = YYEMPTY; // nothing read from token stream
    goto yysetstate;

   yynewstate: // to push a new state onto the stack
    /* In all cases, when you get here, the value and location stacks
       have just been pushed.  So pushing a state here evens the stacks.  */
    yyssp++;  // only increment the pointer - goes to yysetstate naturally

   yysetstate: // actually set the new state pushed onto stack to the value
    *yyssp = yystate;

    /* While pushing we check if the stacks *code ommitted*
    are full, need to be expanded, need relocation and deal with any overflow
    */
    if (yyss + yystacksize - 1 <= yyssp)
      {
        /* Get the current used size of the three stacks, in elements.  */
        YYSIZE_T yysize = yyssp - yyss + 1;
        // code for overflow ommitted
        yyssp = yyss + yysize - 1;
        yyvsp = yyvs + yysize - 1;
        YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                    (unsigned long int) yystacksize));
      }

    YYDPRINTF ((stderr, "Entering state %d\n", yystate));
    if (yystate == YYFINAL) 	// accept if in the final state
      YYACCEPT;

    goto yybackup;

  yybackup: // main code for processing current state HERE

    // Try to perform an action WITHOUT look-ahead - see if default
    yyn = yypact[yystate];
    if (yypact_value_is_default (yyn)) // default reduction if needed
      goto yydefault;

    // check if a token has been looked ahead
    if (yychar == YYEMPTY) // no token looked-ahead at, read one into yychar
      {
        YYDPRINTF ((stderr, "Reading a token: "));
        yychar = yylex ();
      }
    
    if (yychar <= YYEOF) 	// YYEOF = 0 - returned by lexer at end of input
      { // EOF is assigned the smallest number >= means end of parsing
        yychar = yytoken = YYEOF; //set all to EOF
        YYDPRINTF ((stderr, "Now at end of input.\n"));
      }
    else
      { // A valid token other than EOF is ALREADY in yychar
        yytoken = YYTRANSLATE (yychar); // translate token to int
        YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
      }

    // Now yyn = yypact[yystate] + token - refer to double displacement
    yyn += yytoken;
    // checks on yyn - if invalid, do the default action
    if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken) 
      // YLAST is highest index in yytable
      goto yydefault;
    
    yyn = yytable[yyn]; 	// index into yytable - get the state
    if (yyn <= 0) 	    // negative yyn => reduce action
      {
        if (yytable_value_is_error (yyn))
          goto yyerrlab;   // handle errors
        yyn = -yyn; 		// invert value if not erroneous, for reduction no.
        goto yyreduce;
      }

    if (yyerrstatus)
      yyerrstatus--; // count the number of tokens shifted since error

    // Being here means no error, default action or reduction => shift move
    YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

    yychar = YYEMPTY; // clear out the look-ahead token - has been shifted

    // set the current state to computed one 
    // yystate = yytable[yypact[yystate] + yytoken]
    yystate = yyn;
    
    *++yyvsp = yylval; // push semantic value of symbol on its stack
    // pushed new symbol to semantic stack, now push state onto state stack
    goto yynewstate;

  yydefault:		
    yyn = yydefact[yystate];  // get default action for the state
    if (yyn == 0)			   // NO default reduction
      goto yyerrlab;
    goto yyreduce;		   // reduce with the default reduction no.

  yyreduce:
    // yyn has the reduction rule no.
    yylen = yyr2[yyn]; 	  // get the length of RHS of the reduction rule
    yyval = yyvsp[1-yylen]; // semantic value operation eg. $$ = $1 if len = 1

    YY_REDUCE_PRINT (yyn);
    switch (yyn) 			 // match reduction rule to semantic action
      {
  #line 2058 "c.tab.cpp" /* yacc.c:1646  */
        // user specified semantic actions come here - none in the grammar
        default: break;
      }
    YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

    YYPOPSTACK (yylen);	  // pop both the stacks
    yylen = 0;			 // reset length
    YY_STACK_PRINT (yyss, yyssp);

    *++yyvsp = yyval;		 // Push the result of semantic eval. to its stack

    // 'shift' the result of the reduction
    // Determine what state that goes to, based on
    // the state popped back to and the rule no.
    yyn = yyr1[yyn];		// Get the LHS of the reduction rule

    /* State to GOTO, based on symbol reduce to and top of state stack
    - subtract no. of terminals (YYNTOKENS) from symbol no. of nonterminal
    to get an index into yypgoto or yydefgoto for that non-terminal */
    yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
    // checks - yystate within bounds of yytable
    // 	    - state at stack top is valid
    if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
      yystate = yytable[yystate];
    else // default GOTO
      yystate = yydefgoto[yyn - YYNTOKENS];
    goto yynewstate; // new state from GOTO, push onto stack

  yyacceptlab: 	  // accept the input
    yyresult = 0;	  // return without errors
    goto yyreturn;

  yyabortlab:		  // abort parsing
    yyresult = 1;	  // error return value
    goto yyreturn;

  yyreturn:		 // return from parsing to the caller
    if (yychar != YYEMPTY) 
      {			// clean up look-ahead
        yytoken = YYTRANSLATE (yychar);
        yydestruct ("Cleanup: discarding lookahead",
                    yytoken, &yylval);
      }

    YYPOPSTACK (yylen);
    YY_STACK_PRINT (yyss, yyssp);
    while (yyssp != yyss) 
      {			// clean up stack - clear symbols
        yydestruct ("Cleanup: popping",
                    yystos[*yyssp], yyvsp);
        YYPOPSTACK (1);
      }
    return yyresult;
  }
  ```

  * Error Handling function can also be user defined, put in the end of the file

  ```C
  #include <stdio.h>

  void yyerror(const char *s)
  {	// flushes the output stream, and print errors to stderr
  	fflush(stdout);
  	fprintf(stderr, "*** %s\n", s);
  }
  ```

  ​

  ​