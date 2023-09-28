/*
 * DAPPLE, Apple ][, ][+, //e Emulator
 * Copyright 2002, 2003 Steve Nickolas, portions by Holger Picker
 *
 * Component:  I18N: Internationalization function
 * Revision:   (1.20) 2002.1229
 *
 * Foreign character translation derived from experimentation and Epson
 * RX-80 documentation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

static char xchr[11][12]={
 {'#', '$', '@', '[', '\\',']', '^', '`', '{', '|', '}', '~'}, /* USA       */
 {'#', '$', 133, 248, 135, 232, '^', '`', 130, 151, 138, 249}, /* France    */
 {'#', '$', 232, 142, 143, 144, '^', '`', 132, 148, 129, 225}, /* Germany   */
 {156, '$', '@', '[', '\\',']', '^', '`', '{', '|', '}', '~'}, /* UK        */
 {'#', '$', '@', 146, 242, 143, '^', '`', 145, 243, 134, '~'}, /* Denmark 1 */
 {'#', 241, 144, 142, 153, 143, 154, 130, 132, 148, 134, 129}, /* Sweden    */
 {'#', '$', '@', 248, '\\',130, '^', 151, 133, 149, 138, 141}, /* Italy     */
 {158, '$', '@', 173, 165, 168, '^', '`', 249, 164, '}', '~'}, /* Spain     */
 {'#', '$', '@', '[', 157, ']', '^', '`', '{', '|', '}', '~'}, /* Japan     */
 {'#', 241, 144, 146, 242, 143, 154, 130, 145, 243, 134, 129}, /* Norway    */
 {'#', '$', 144, 146, 242, 143, 154, 130, 145, 243, 134, 129}, /* Denmark 2 */
};

int chartrans (int mode, int mychar)
{
  int tmp;

  tmp=mychar;

  if (mychar=='#') tmp=xchr[mode][0];
  if (mychar=='$') tmp=xchr[mode][1];
  if (mychar=='@') tmp=xchr[mode][2];
  if (mychar=='[') tmp=xchr[mode][3];
  if (mychar=='\\') tmp=xchr[mode][4];
  if (mychar==']') tmp=xchr[mode][5];
  if (mychar=='^') tmp=xchr[mode][6];
  if (mychar=='`') tmp=xchr[mode][7];
  if (mychar=='{') tmp=xchr[mode][8];
  if (mychar=='|') tmp=xchr[mode][9];
  if (mychar=='}') tmp=xchr[mode][10];
  if (mychar=='~') tmp=xchr[mode][11];

  return tmp;
}
