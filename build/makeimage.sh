dd if=/dev/zero of=$1 bs=1024 count=1440
mkfs.msdos $1
mkdir -p mnt
mount -oloop $1 mnt

mkdir mnt/grub
cp /boot/grub/stage? mnt/grub

echo timeout 2 >> mnt/grub/menu.lst
echo title LOST >> mnt/grub/menu.lst
echo kernel /lost.krn >> mnt/grub/menu.lst
echo module /init.mod >> mnt/grub/menu.lst
for i in *.mod; do 
    [ "$i" != "init.mod" ] && echo module /"$i" >> mnt/grub/menu.lst
done

echo boot >> mnt/grub/menu.lst

cp *.krn mnt
cp *.mod mnt
umount mnt
rmdir mnt

echo device '(fd0)' lost.img > grub.txt
echo root '(fd0)' >> grub.txt
echo setup '(fd0)' >> grub.txt
grub --batch < grub.txt
rm grub.txt
