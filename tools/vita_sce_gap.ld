/* vita-elf-create appends SCE module metadata to the executable segment.
 * Reserve a minimum gap before writable data, independent of program size. */
SECTIONS
{
  . = ALIGN(0x1000) + 0x4000;
}
INSERT BEFORE .init_array;
