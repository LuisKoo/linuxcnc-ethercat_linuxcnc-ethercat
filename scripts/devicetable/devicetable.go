package main

import (
	"flag"
	"fmt"
	"gopkg.in/yaml.v3"
	"log"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"strconv"
)

type DeviceDefinition struct {
	Device           string `yaml:"Device"`
	VendorID         string `yaml:"VendorID"`
	VendorName       string `yaml:"VendorName"`
	PID              string `yaml:"PID"`
	Description      string `yaml:"Description"`
	DocumentationURL string `yaml:"DocumentationURL"`
	DeviceType       string `yaml:"DeviceType"`
	Channels         int    `yaml:"Channels"`
	Notes            string `yaml:"Notes"`
	SrcFile          string `yaml:"SrcFile"`
	TestingStatus    string `yaml:"TestingStatus"`
}

var pathFlag = flag.String("path", "../../documentation/devices/", "Path to *.yml files for device documentation")

func parsefile(filename string) (*DeviceDefinition, error) {
	data, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("unable to read file: %v", filename)
	}

	entry := &DeviceDefinition{}

	err = yaml.Unmarshal(data, entry)
	if err != nil {
		return nil, fmt.Errorf("unable to decode yaml: %v", err)
	}

	return entry, nil
}

func main() {
	flag.Parse()

	entries := []*DeviceDefinition{}
	otherfiles := 0

	files, err := os.ReadDir(*pathFlag)
	if err != nil {
		log.Fatalf("Unable to read directory %q: %v", *pathFlag, err)
	}

	filenames := []string{}
	for _, file := range files {
		if strings.HasSuffix(file.Name(), ".yml") {
			filenames = append(filenames, file.Name())
		}
	}

	sort.Strings(filenames)
	for _, name := range filenames {
		if name[0] != '.' && strings.Contains(name, ".yml") {
			entry, err := parsefile(filepath.Join(*pathFlag, name))
			if err != nil {
				log.Fatalf("Unable to parse file %q: %v", name, err)
			}

			// The `devicelist.sh` script autogenerates entries
			// with `Device: TODO`.  For now, let's filter them
			// out.
			//
			// Longer-term, we may wish to gripe when we find
			// them, or replace `TODO` with `???` or similar.
			if strings.TrimSpace(entry.Description) != "UNKNOWN" {
				entries = append(entries, entry)
			} else {
				otherfiles++
			}
		}
	}

	fmt.Printf("# Devices Supported by LinuxCNC-Ethercat\n")
	fmt.Printf("\n")
	fmt.Printf("*This is a work in progress, listing all of the devices that LinuxCNC-Ethercat\n")
	fmt.Printf("has code to support today.  Not all of these are well-tested.*\n")
	fmt.Printf("\n")

	fmt.Printf("Description | Driver | EtherCAT VID:PID | Device Type | Testing Status | Notes\n")
	fmt.Printf("----------- | ------ | ---------------- | ----------- | -------------- | ------\n")
	for _, e := range entries {
		name := e.Description
		if len(e.DocumentationURL) > 4 && e.DocumentationURL[0:4] == "http" {
			name = fmt.Sprintf("[%s](%s)", e.Description, e.DocumentationURL)
		}
		srcName := "???"
		if e.SrcFile != "" {
			split := strings.Split(e.SrcFile,"/")
			shortname := split[len(split)-1]
			shortname = strings.ReplaceAll(shortname, ".c", "")
			shortname = strings.ReplaceAll(shortname, "lcec_", "")
			srcName = fmt.Sprintf("[%s](../%s)", shortname, e.SrcFile)
		}
		vid, _ := strconv.ParseInt(e.VendorID, 0, 64)
		pid := e.PID
		fmt.Printf("%s | %s | 0x%x:%s | %s | %s | %s\n", name, srcName, vid, pid, e.DeviceType, e.TestingStatus, e.Notes)
	}

	fmt.Printf("\n")
	if otherfiles > 0 {
		fmt.Printf("There are an additional %d device(s) supported that do not have enough\n", otherfiles)
		fmt.Printf("documentation to display here.  Please look at the `documentation/devices/` files\n")
		fmt.Printf("and update them if you're able.\n")
	}
}
