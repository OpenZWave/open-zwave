#!/usr/bin/perl

# use module
use strict;
use XML::Simple;
use Data::Dumper;
use Getopt::Long qw(GetOptions);

my %errors = ();
my %warnings = ();

sub LogError {
    if (CheckSuppression($_[0], $_[1])) {
        return;
    }

    my $errordetail;
    $errordetail->{'file'} = $_[0];
    $errordetail->{'code'} = $_[1];
    $errordetail->{'description'} = $_[2];
    #print Dumper($errordetail);
    push(@{$errors{$_[0]}}, $errordetail);
}

sub LogWarning {
    if (CheckSuppression($_[0], $_[1])) {
        return;
    }

    my $warningdetail;
    $warningdetail->{'file'} = $_[0];
    $warningdetail->{'code'} = $_[1];
    $warningdetail->{'description'} = $_[2];
    push(@{$warnings{$_[0]}}, $warningdetail);

}

# check common config file mistakes 
sub CheckConfig {
    use strict;
    use warnings;
    my $file = $_[0];
    my $count = 1;
    open my $info, $file or die "Could not open $file: $!";
    while( my $line = <$info>)  {
        if ($line =~ /[[:^ascii:]]/) {
            LogError($file, 5, "Line $count, contains non ASCII characters");
        }
        ++$count;
    }
    close $info;

    # create object
    my $xml = new XML::Simple;
    # read XML file
    my $data = $xml->XMLin($_[0], ForceArray => [ 'Group' ]);
    # print output
    #print Dumper($data->{CommandClass}->{133});
    foreach my $group ($data->{CommandClass}->{133}->{Associations}->{Group}) {
        if (defined($group)) {
            my $arrSize = @{$group};
            if ($arrSize != $data->{CommandClass}->{133}->{Associations}->{num_groups}) {
                LogError($_[0], 4, "Number of Groups does not equal Group Entries");
            }
            foreach my $entry (@{$group}) {
                if ((defined($entry->{auto}))
                 && ($entry->{index} == 1)
                  && (lc $entry->{auto} eq "true")) {
                    LogError($_[0], 1,"Association Group 1 has auto equal to true");
                }
                if ((defined($entry->{auto}))
                && ($entry->{index} != 1)
                && (lc $entry->{auto} eq "false")) {
                    LogError($_[0], 2, "Association Group $entry->{index} has auto set to False");
                }
            }
        } else {
            LogWarning($_[0], 3, "No Association Groups Defined for device");
        }
    }
    $data = $xml->XMLin($_[0], ForceArray => [ 'Value' ]);
    # print output
    foreach my $valueItem ($data->{CommandClass}->{112}->{Value}) {
        if (defined($valueItem)) {
            foreach my $configuration (@{$valueItem}) {
                if ((defined($configuration->{type})) && (lc $configuration->{type} eq "list") && (not defined($configuration->{size}))) {
                    LogError($_[0], 2, "Configuration of type list $configuration->{index} size not defined");
                }
             }
        }
    }
}

# check files match entries in manufacture_specific.xml 

sub CheckFileExists {
    my %configfiles = map { lc $_ => 1} @{$_[0]};
    # create object
    my $xml = new XML::Simple;

    # read XML file
    my $data = $xml->XMLin("config/manufacturer_specific.xml", KeyAttr => "", ForceArray => [ 'Product' ] );
    foreach my $manu (@{$data->{Manufacturer}}) {
        if (defined($manu->{Product})) {
            foreach my $config (@{$manu->{Product}}) {
                if (defined($config->{config})) {
                    #print Dumper($config->{config});
                    if (!-e "config/$config->{config}") {
                        LogError("manufacturer_specific.xml", 5, "Config File config/$config->{config} Specified in manufacturer_specific.xml doesn't exist");
                    } else {
                        delete $configfiles{lc "config/$config->{config}"};
                    }
                }
            }
        }
    }
    #anything left in $configfiles hasn't been specified in manufacturer_specific.xml
    #print Dumper(%configfiles);
    foreach my $unreffile (keys %configfiles) {
        LogWarning("manufacturer_specific.xml", 7, "Unreferenced Config File $unreffile present on file system");
    }
}

sub PrettyPrintErrors() {
	print "\n\nErrors: (Please Correct before Submitting to OZW)\n";
	while ((my $key, my $value) = each %errors) {
		foreach my $detail (@{$value}) {
			print $key.": ".$detail->{description}." - Error Code $detail->{code}\n";
		}
		print "\n";
	}
}

sub PrettyPrintWarnings() {
	print "\n\nWarnings: (Can be Ignored)\n";
	while ((my $key, my $value) = each %warnings) {
		foreach my $detail (@{$value}) {
			print $key.": ".$detail->{description}." - Warning Code $detail->{code}\n";
		}
		print "\n";
	}
}

sub XMLPrintErrors() {
	my $numerrs  = 0;
	while ((my $key, my $value) = each %errors)	{
		foreach my $detail (@{$value}) {
			$numerrs++;
		}
	}
	open(my $fh, '>', 'results/OZW_CheckConfig.xml') or die "Could not open file results\OZW_CheckConfig.xml $!";
	print $fh "<testsuite failures=\"0\" assertions=\"\" name=\"OZW_CheckConfig\" tests=\"1\" errors=\"$numerrs\" time=\"\">\n";
	while ((my $key, my $value) = each %errors)	{
		foreach my $detail (@{$value}) {
			print $fh "\t<testcase assertions=\"1\" name=\"$detail->{code}-$detail->{file}\" time=\"\">\n";
			print $fh "\t\t<failure type=\"ScriptError\" message=\"$detail->{description}\"></failure>\n";
			print $fh "\t\t<system-out>\n";
			print $fh "\t\t<![CDATA[File: $detail->{file}\nDescription: $detail->{description}\nError Code: $detail->{code}]]>\n";
			print $fh "\t\t</system-out>\n";
			print $fh "\t</testcase>\n";
		}
	}
	print $fh "</testsuite>\n";
	close $fh;
}

sub XMLPrintWarnings() {
	my $numerrs  = 0;
	while ((my $key, my $value) = each %warnings) {
		foreach my $detail (@{$value}) {
			$numerrs++;
		}
	}
	open(my $fh, '>', 'results/OZW_CheckConfigWarnings.xml') or die "Could not open file results\OZW_CheckConfig.xml $!";
	print $fh "<testsuite failures=\"0\" assertions=\"\" name=\"OZW_CheckConfigWarnings\" tests=\"1\" errors=\"$numerrs\" time=\"\">\n";
	while ((my $key, my $value) = each %warnings) {
		foreach my $detail (@{$value}) {
			print $fh "\t<testcase assertions=\"1\" name=\"$detail->{code}-$detail->{file}\" time=\"\">\n";
			print $fh "\t\t<failure type=\"ScriptError\" message=\"$detail->{description}\"></failure>\n";
			print $fh "\t\t<system-out>\n";
			print $fh "\t\t<![CDATA[File: $detail->{file}\nDescription: $detail->{description}\nError Code: $detail->{code}]]>\n";
			print $fh "\t\t</system-out>\n";
			print $fh "\t</testcase>\n";
		}
	}
	print $fh "</testsuite>\n";
	close $fh;
}

# Read a configuration file
#   The arg can be a relative or full path, or
#   it can be a file located somewhere in @INC.
sub ReadCfg {
    my $file = "cpp/build/testconfigsuppressions.cfg";
    our $err;
    {   # Put config data into a separate namespace
        package CFG;
        # Process the contents of the config file
        my $rc = do($file);
        # Check for errors
        if ($@) {
            $::err = "ERROR: Failure compiling '$file' - $@";
        } elsif (! defined($rc)) {
            $::err = "ERROR: Failure reading '$file' - $!";
        } elsif (! $rc) {
            $::err = "ERROR: Failure processing '$file'";
        }
    }
    return ($err);
}

sub CheckSuppression {
	my $file = $_[0];
	my $code = $_[1];
	if (defined($CFG::CFG{$file}) && $CFG::CFG{$file}{'code'} == $code) {
		return 1
	}
	return;
}

my $doxml;
my $printwarnings;
GetOptions(	"printwarnings" => \$printwarnings,
			"outputxml" => \$doxml
			) or die("Error in Command Line arguements\n");

if (my $err = ReadCfg()) {
    print(STDERR $err, "\n");
    exit(1);
}

print "Checking Config Files... Please Wait\n";
my $dirname="config";
opendir(DIR, $dirname);
my @dirs = readdir(DIR);
closedir DIR;
my @filelist;
foreach my $key (@dirs) {
 	next if ($key eq "."); 
  	next if ($key eq "..");
  	if(-d "$dirname/$key") {
  		my @files = glob("$dirname/$key/*.xml");
   		foreach my $file (@files) {
    		next if ($file eq "."); 
    		next if ($file eq "..");
			push(@filelist, $file);
    		#print "Checking $file\n";    
    		CheckConfig("$file");
   		}
  	}
} 

CheckFileExists(\@filelist);

if ($doxml == 0) { 
	PrettyPrintErrors();
}
if ($doxml == 0 && $printwarnings == 1) {
	PrettyPrintWarnings();
}
if ($doxml == 1) {
	XMLPrintErrors();
}
if ($doxml == 1 && $printwarnings == 1) {
	XMLPrintWarnings();
}