--TEST--
Check for wordutil presence
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded("wordutil")) print "skip"; ?>
--FILE--
<?php 
var_dump(replace_word("百度阿里腾讯头条滴滴小米oppo"));
/*
	you can add regression tests for your extension here

  the output of your test code has to be equal to the
  text in the --EXPECT-- section below for the tests
  to pass, differences between the output and the
  expected text are interpreted as failure

	see php5/README.TESTING for further information on
  writing regression tests
*/
?>
--EXPECT--
string(37) "百度***腾讯头条******小米oppo"
